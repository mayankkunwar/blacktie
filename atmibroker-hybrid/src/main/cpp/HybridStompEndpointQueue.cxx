/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#include <exception>

#include "apr_strings.h"
#include "malloc.h"
#include "ThreadLocalStorage.h"
#include "txx.h"
#include "HybridStompEndpointQueue.h"
#include "HybridConnectionImpl.h"
#include "AtmiBrokerEnv.h"
#include "BufferConverterImpl.h"

log4cxx::LoggerPtr HybridStompEndpointQueue::logger(log4cxx::Logger::getLogger(
		"HybridStompEndpointQueue"));

HybridStompEndpointQueue::HybridStompEndpointQueue(apr_pool_t* pool,
		char* serviceName, bool conversational) {
	LOG4CXX_DEBUG(logger, "Creating endpoint queue: " << serviceName);
	this->message = NULL;
	this->receipt = NULL;
	_connected = false;
	shutdown = false;
	requiresDisconnect = false;
	shutdownLock = new SynchronizableObject();
	LOG4CXX_DEBUG(logger, "Created lock: " << shutdownLock);
	readLock = new SynchronizableObject();
	LOG4CXX_DEBUG(logger, "Created lock: " << readLock);
	this->pool = pool;

	// XATMI_SERVICE_NAME_LENGTH is in xatmi.h and therefore not accessible
	int XATMI_SERVICE_NAME_LENGTH = 15;
	int queueNameLength = 14 + 15 + 1;
	char* queueName = (char*) ::malloc(queueNameLength);
	memset(queueName, '\0', queueNameLength);
	if (conversational) {
		strcpy(queueName, "/queue/BTC_");
	} else {
		strcpy(queueName, "/queue/BTR_");
	}
	strncat(queueName, serviceName, XATMI_SERVICE_NAME_LENGTH);
	this->fullName = queueName;
	this->name = strdup(serviceName);
	this->transactional = true;
	this->unackedMessages = 0;
}

// ~EndpointQueue destructor.
//
HybridStompEndpointQueue::~HybridStompEndpointQueue() {
	LOG4CXX_TRACE(logger, (char*) "destroying" << name);

	disconnect();

	// Read
	readLock->lock();

	LOG4CXX_TRACE(logger, (char*) "deleting lock");
	delete readLock;
	readLock = NULL;

	// Reacquire the lock so we know its safe to delete it
	shutdownLock->lock();
	LOG4CXX_TRACE(logger, (char*) "deleting lock");
	delete shutdownLock;
	shutdownLock = NULL;
	LOG4CXX_TRACE(logger, (char*) "freeing name" << name);
	free( name);
	free( fullName);
	LOG4CXX_TRACE(logger, (char*) "freed name");
	LOG4CXX_TRACE(logger, (char*) "destroyed");
}

MESSAGE HybridStompEndpointQueue::receive(long time) {
	// TODO TIME NOT RESPECTED
	MESSAGE message;
	message.replyto = NULL;
	message.correlationId = -1;
	message.data = NULL;
	message.len = -1;
	message.priority = 0;
	message.flags = -1;
	message.control = NULL;
	message.rval = -1;
	message.rcode = -1;
	message.type = NULL;
	message.subtype = NULL;
	message.received = false;
	message.ttl = -1;
	message.serviceName = NULL;
	message.messageId = NULL;

	setSpecific(TPE_KEY, TSS_TPERESET);

	if (!shutdown) {
		readLock->lock();
		if (!shutdown) {
			stomp_frame *frame = NULL;
			connect();
			if (_connected) {
				if (this->message) {
					LOG4CXX_DEBUG(logger, "Handing off old message");
					frame = this->message;
					this->message = NULL;
				} else {
					LOG4CXX_TRACE(logger, (char*) "Receiving from: " << name);
					apr_status_t rc = stomp_read(connection, &frame, pool);
					if (rc == APR_TIMEUP || rc == 730060) {
						LOG4CXX_TRACE(logger, "Could not read frame for "
								<< name << ": as the time limit expired");
						setSpecific(TPE_KEY, TSS_TPETIME);
						frame = NULL;
					} else if (rc != APR_SUCCESS) { // win32 70014 on disconnect
						LOG4CXX_DEBUG(logger, "Could not read frame for "
								<< name);
						//						shutdownLock->lock(); NOT CLEAR WHY WE NEED TO LOCK SHUTDOWN - IS IT REALLY TO CHECK THE SHUTDOWN FLAG FOR LOGGING??
						char errbuf[256];
						apr_strerror(rc, errbuf, sizeof(errbuf));
						if (!shutdown) {
							LOG4CXX_WARN(logger, (char*) "APR Error was: "
									<< rc << ": " << errbuf);
						} else {
							LOG4CXX_DEBUG(logger, (char*) "APR Error was: "
									<< rc << ": " << errbuf);
						}
						//					free(errbuf);
						setSpecific(TPE_KEY, TSS_TPESYSTEM);
						frame = NULL;
						this->_connected = false;
						//						shutdownLock->unlock();
					} else if (strcmp(frame->command, (const char*) "ERROR")
							== 0) {
						LOG4CXX_ERROR(logger, (char*) "Got an error: "
								<< frame->body);
						setSpecific(TPE_KEY, TSS_TPENOENT);
						frame = NULL;
					} else if (strcmp(frame->command, (const char*) "RECEIPT")
							== 0) {
						char * receipt = (char*) apr_hash_get(frame->headers,
								"receipt-id", APR_HASH_KEY_STRING);
						if (this->receipt == NULL || strcmp(this->receipt,
								receipt) != 0) {
							LOG4CXX_ERROR(logger,
									(char*) "read an unexpected receipt for: "
											<< name << ": " << receipt);
							setSpecific(TPE_KEY, TSS_TPESYSTEM);
							frame = NULL;
							this->_connected = false;
						} else {
							LOG4CXX_DEBUG(logger, "Handling old receipt: "
									<< receipt);
							this->receipt = NULL;
							rc = stomp_read(connection, &frame, pool);
							if (rc == APR_TIMEUP || rc == 730060) {
								LOG4CXX_TRACE(
										logger,
										"Could not read frame for " << name
												<< ": as the time limit expired");
								setSpecific(TPE_KEY, TSS_TPETIME);
								frame = NULL;
							} else if (rc != APR_SUCCESS) {
								LOG4CXX_ERROR(logger,
										"Could not read frame for " << name);
								char errbuf[256];
								apr_strerror(rc, errbuf, sizeof(errbuf));
								LOG4CXX_DEBUG(logger, (char*) "APR Error was: "
										<< rc << ": " << errbuf);
								//							free(errbuf);
								setSpecific(TPE_KEY, TSS_TPESYSTEM);
								frame = NULL;
								this->_connected = false;
							} else if (strcmp(frame->command,
									(const char*) "ERROR") == 0) {
								LOG4CXX_ERROR(logger, (char*) "Got an error: "
										<< frame->body);
								setSpecific(TPE_KEY, TSS_TPENOENT);
								frame = NULL;
							} else if (strcmp(frame->command,
									(const char*) "RECEIPT") == 0) {
								char * receipt = (char*) apr_hash_get(
										frame->headers, "receipt-id",
										APR_HASH_KEY_STRING);
								LOG4CXX_ERROR(logger,
										(char*) "read a RECEIPT for: " << name
												<< ": " << receipt);
								setSpecific(TPE_KEY, TSS_TPESYSTEM);
								frame = NULL;
								this->_connected = false;
							} else {
								LOG4CXX_DEBUG(logger,
										"Message received 2nd attempt");
							}
						}
					} else {
						LOG4CXX_DEBUG(logger, "Message received 1st attempt");
					}
				}
				if (frame != NULL) {
					shutdownLock->lock();
					// WONT BE ABLE TO SEND AN ACK SO IGNORE MESSAGE
					if (!shutdown) {
						LOG4CXX_DEBUG(logger, "Received from: " << name
								<< " Command: " << frame->command);
						message.messageId = (char*) apr_hash_get(
								frame->headers, "message-id",
								APR_HASH_KEY_STRING);
						message.control = (char*) apr_hash_get(frame->headers,
								"messagecontrol", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted control: "
								<< message.control);

						LOG4CXX_TRACE(logger, "Ready to handle message");
						char * correlationId = (char*) apr_hash_get(
								frame->headers, "messagecorrelationId",
								APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Read a correlation ID"
								<< correlationId);
						LOG4CXX_TRACE(logger, "Extracted correlationID");
						char * flags = (char*) apr_hash_get(frame->headers,
								"messageflags", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted flags");
						char * rval = (char*) apr_hash_get(frame->headers,
								"messagerval", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted rval");
						char * rcode = (char*) apr_hash_get(frame->headers,
								"messagercode", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted rcode");

						char * type = (char*) apr_hash_get(frame->headers,
								"messagetype", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted messagetype");
						message.type = type;

						char * subtype = (char*) apr_hash_get(frame->headers,
								"messagesubtype", APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted messagesubtype");
						message.subtype = subtype;

						char * serviceName = (char*) apr_hash_get(
								frame->headers, "servicename",
								APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Extracted servicename");

						message.len = frame->body_length;
						message.data
								= BufferConverterImpl::convertToMemoryFormat(
										message.type, message.subtype,
										frame->body, &message.len);
						LOG4CXX_TRACE(logger, "Set body and length: "
								<< message.len);

						message.replyto = (const char*) apr_hash_get(
								frame->headers, "messagereplyto",
								APR_HASH_KEY_STRING);
						LOG4CXX_TRACE(logger, "Set replyto: "
								<< message.replyto);
						message.correlationId = apr_atoi64(correlationId);
						LOG4CXX_TRACE(logger, "Set correlationId: "
								<< message.correlationId);
						message.flags = apr_atoi64(flags);
						LOG4CXX_TRACE(logger, "Set flags: " << message.flags);
						message.rval = apr_atoi64(rval);
						LOG4CXX_TRACE(logger, "Set rval: " << message.rval);
						message.rcode = apr_atoi64(rcode);
						LOG4CXX_TRACE(logger, "Set rcode: " << message.rcode);
						LOG4CXX_TRACE(logger, "Set control: "
								<< message.control);
						message.serviceName = serviceName;
						LOG4CXX_TRACE(logger, "set serviceName");
						message.received = true;
						unackedMessages++;
					} else {
						LOG4CXX_WARN(logger,
								"Receive message not returned as shutdown");
					}
					shutdownLock->unlock();

				}
			} else {
				if (!shutdown) {
					LOG4CXX_ERROR(logger,
							"receive failed - not able to connect");
				} else {
					LOG4CXX_DEBUG(logger, "receive failed - in shutdown");
				}
			}
		} else {
			LOG4CXX_DEBUG(logger, "receive failed - in shutdown");
		}
		readLock->unlock();
	}
	return message;
}

bool HybridStompEndpointQueue::connected() {
	LOG4CXX_DEBUG(logger, (char*) "connected: " << name);
	return _connected;
}

void HybridStompEndpointQueue::disconnect() {
	LOG4CXX_DEBUG(logger, (char*) "disconnecting: " << name);
	shutdownLock->lock();
	if (!this->shutdown) {
		// Always set shutdown to true as we are shutting down
		this->shutdown = true;
		LOG4CXX_DEBUG(logger, (char*) "Shutdown set: " << name);
		if (this->_connected) {
			this->_connected = false;
			// This will only disconnect after the last message is consumed
			requiresDisconnect = true;
			disconnectImpl();
		}
	}
	shutdownLock->unlock();
	LOG4CXX_DEBUG(logger, (char*) "disconnected: " << name);
}

void HybridStompEndpointQueue::disconnectImpl() {
	LOG4CXX_DEBUG(logger, (char*) "disconnecting: " << name);
	//	shutdownLock->lock(); NOT REQUIRED AS WE SHOULD ALWAYS HAVE THE LOCK
	if (requiresDisconnect && unackedMessages == 0) {
		stomp_frame frame;
		frame.command = (char*) "DISCONNECT";
		frame.headers = NULL;
		frame.body_length = -1;
		frame.body = NULL;
		LOG4CXX_TRACE(logger, (char*) "Sending DISCONNECT" << connection
				<< "pool" << pool);
		apr_status_t rc = stomp_write(connection, &frame, pool);
		LOG4CXX_TRACE(logger, (char*) "Sent DISCONNECT");
		if (rc != APR_SUCCESS) {
			LOG4CXX_ERROR(logger, "Could not send frame");
			char errbuf[256];
			apr_strerror(rc, errbuf, sizeof(errbuf));
			LOG4CXX_ERROR(logger, (char*) "APR Error was: " << rc << ": "
					<< errbuf);
			//			free(errbuf);
		}

		// Disconnect existing receivers
		rc = apr_socket_shutdown(connection->socket, APR_SHUTDOWN_WRITE);
		LOG4CXX_TRACE(logger, (char*) "Sent SHUTDOWN");
		if (rc != APR_SUCCESS) {
			LOG4CXX_ERROR(logger, "Could not send SHUTDOWN");
			char errbuf[256];
			apr_strerror(rc, errbuf, sizeof(errbuf));
			LOG4CXX_ERROR(logger, (char*) "APR Error was: " << rc << ": "
					<< errbuf);
			//			free(errbuf);
		}

		readLock->lock();
		LOG4CXX_DEBUG(logger, "Disconnecting...");
		rc = stomp_disconnect(&connection);
		if (rc != APR_SUCCESS) {
			LOG4CXX_ERROR(logger, "Could not disconnect");
			char errbuf[256];
			apr_strerror(rc, errbuf, sizeof(errbuf));
			LOG4CXX_ERROR(logger, (char*) "APR Error was: " << rc << ": "
					<< errbuf);
			//			free(errbuf);
		} else {
			LOG4CXX_DEBUG(logger, "Disconnected");
		}
		readLock->unlock();
		requiresDisconnect = false;
	}
	//	shutdownLock->unlock(); SEE NOTE ABOVE RE ALWAYS HAVING THE LOCK
	LOG4CXX_DEBUG(logger, (char*) "disconnected: " << name);
}

bool HybridStompEndpointQueue::connect() {
	shutdownLock->lock();
	if (!shutdown && !_connected) {
		LOG4CXX_DEBUG(logger, (char*) "connecting: " << fullName);
		this->connection = HybridConnectionImpl::connect(pool,
				mqConfig.destinationTimeout);
		if (this->connection != NULL) {
			readLock->lock();
			stomp_frame frame;
			frame.command = (char*) "SUBSCRIBE";
			frame.headers = apr_hash_make(pool);
			apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING,
					fullName);
			apr_hash_set(frame.headers, "receipt", APR_HASH_KEY_STRING,
					fullName);
			apr_hash_set(frame.headers, "ack", APR_HASH_KEY_STRING, "client");
			frame.body_length = -1;
			frame.body = NULL;
			LOG4CXX_DEBUG(logger, "Sending SUB: " << fullName);
			apr_status_t rc = stomp_write(connection, &frame, pool);
			if (rc != APR_SUCCESS) {
				LOG4CXX_ERROR(logger, (char*) "Could not send frame");
				char errbuf[256];
				apr_strerror(rc, errbuf, sizeof(errbuf));
				LOG4CXX_ERROR(logger, (char*) "APR Error was: " << rc << ": "
						<< errbuf);
				HybridConnectionImpl::disconnect(connection, pool);
			} else {
				stomp_frame *framed;
				LOG4CXX_DEBUG(logger, "Reading response: " << fullName);
				rc = stomp_read(connection, &framed, pool);
				if (rc != APR_SUCCESS) {
					setSpecific(TPE_KEY, TSS_TPESYSTEM);
					LOG4CXX_ERROR(logger, "Could not read frame for " << name);
					char errbuf[256];
					apr_strerror(rc, errbuf, sizeof(errbuf));
					LOG4CXX_ERROR(logger, (char*) "APR Error was: " << rc
							<< ": " << errbuf);
					HybridConnectionImpl::disconnect(connection, pool);
				} else if (strcmp(framed->command, (const char*) "ERROR") == 0) {
					setSpecific(TPE_KEY, TSS_TPENOENT);
					LOG4CXX_ERROR(logger, (char*) "Got an error: "
							<< framed->body);
					HybridConnectionImpl::disconnect(connection, pool);
				} else if (strcmp(framed->command, (const char*) "RECEIPT")
						== 0) {
					LOG4CXX_DEBUG(logger, (char*) "Got a receipt: "
							<< (char*) apr_hash_get(framed->headers,
									"receipt-id", APR_HASH_KEY_STRING));
					this->_connected = true;
					LOG4CXX_DEBUG(logger, "Connected: " << fullName);
				} else if (strcmp(framed->command, (const char*) "MESSAGE")
						== 0) {
					LOG4CXX_DEBUG(
							logger,
							(char*) "Got message before receipt, allow a single receipt later");
					this->message = framed;
					this->receipt = fullName;
					this->_connected = true;

					LOG4CXX_DEBUG(logger, "Connected: " << fullName);
				} else {
					setSpecific(TPE_KEY, TSS_TPESYSTEM);
					LOG4CXX_ERROR(logger,
							"Didn't get a receipt or message unexpected error: "
									<< framed->command << ", " << framed->body);
					HybridConnectionImpl::disconnect(connection, pool);
				}
			}
			readLock->unlock();
		} else {
			setSpecific(TPE_KEY, TSS_TPESYSTEM);
			LOG4CXX_DEBUG(logger, "Not connected");
		}
	}
	shutdownLock->unlock();

	return _connected;
}

void HybridStompEndpointQueue::ack(MESSAGE message) {
	char* messageId = message.messageId;
	LOG4CXX_DEBUG(logger, "Sending ACK: " << messageId);
	shutdownLock->lock();

	stomp_frame ackFrame;
	ackFrame.command = (char*) "ACK";
	ackFrame.headers = apr_hash_make(pool);
	apr_hash_set(ackFrame.headers, "message-id", APR_HASH_KEY_STRING, messageId);
	ackFrame.body = NULL;
	ackFrame.body_length = -1;
	LOG4CXX_DEBUG(logger, "Acking: " << messageId);
	int rc = stomp_write(connection, &ackFrame, pool);
	if (rc != APR_SUCCESS) {
		LOG4CXX_ERROR(logger, (char*) "Could not send frame");
		char errbuf[256];
		apr_strerror(rc, errbuf, sizeof(errbuf));
		LOG4CXX_FATAL(logger, (char*) "APR Error was: " << rc << ": " << errbuf);
		connection = NULL;
	} else {
		LOG4CXX_DEBUG(logger, "Acked: " << messageId);
	}
	unackedMessages--;
	// This will clean up the queue if we require the disconnect
	disconnectImpl();
	shutdownLock->unlock();
}

const char * HybridStompEndpointQueue::getName() {
	return (const char *) name;
}

const char * HybridStompEndpointQueue::getFullName() {
	return (const char *) this->fullName;
}

bool HybridStompEndpointQueue::isShutdown() {
	bool toReturn = false;
	shutdownLock->lock();
	toReturn = this->shutdown;
	shutdownLock->unlock();
	return toReturn;
}
