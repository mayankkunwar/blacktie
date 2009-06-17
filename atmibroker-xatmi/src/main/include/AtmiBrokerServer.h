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

// Class: AtmiBrokerServer
// A POA servant which implements of the AtmiBroker::Server interface
//


#ifndef BLACKTIE_SERVERSERVERIMPL_H_
#define BLACKTIE_SERVERSERVERIMPL_H_

#include <iostream>
#include <vector>

#ifdef TAO_COMP
#include "AtmiBrokerS.h"
#endif
#include "CorbaConnection.h"
#include "Connection.h"

#include "xatmi.h"
#include "AtmiBrokerEnvXml.h"
#include "ConnectionManager.h"
#include "Destination.h"
#include "ServiceDispatcher.h"

struct _service_data {
	Destination* destination;
	void (*func)(TPSVCINFO *);
	std::vector<ServiceDispatcher*> dispatchers;
	//SVCINFO serviceInfo;
	ServiceInfo* serviceInfo;
};
typedef _service_data ServiceData;
class AtmiBrokerServer: public virtual POA_AtmiBroker::Server {
public:
	AtmiBrokerServer();

	virtual ~AtmiBrokerServer();

	// IDL operations
	//
	virtual CORBA::Short server_init() throw (CORBA::SystemException );

	virtual void server_done() throw (CORBA::SystemException );

	virtual AtmiBroker::ServerInfo*
	get_server_info() throw (CORBA::SystemException );

	virtual AtmiBroker::ServiceInfoSeq*
	get_all_service_info() throw (CORBA::SystemException );

	virtual AtmiBroker::EnvVariableInfoSeq*
	get_environment_variable_info() throw (CORBA::SystemException );

	virtual void set_server_descriptor(const char* xml_descriptor) throw (CORBA::SystemException );

	virtual void set_service_descriptor(const char* service_name, const char* xml_descriptor) throw (CORBA::SystemException );

	virtual void set_environment_descriptor(const char* xml_descriptor) throw (CORBA::SystemException );

	virtual void stop_service(const char* service_name) throw (CORBA::SystemException );

	virtual void start_service(const char* service_name) throw (CORBA::SystemException );

	virtual char * getServerName();

	virtual bool advertiseService(char * serviceName, void(*func)(TPSVCINFO *));

	virtual void unadvertiseService(char * serviceName);

	BLACKTIE_XATMI_DLL bool isAdvertised(char * serviceName);
	void advertiseAtBootime();
	int block();

private:
	Connection* serverConnection;
	CORBA_CONNECTION* realConnection;
	void (*getServiceMethod(const char * aServiceName))(TPSVCINFO *);
	void addDestination(Destination* destination, void(*func)(TPSVCINFO *), ServiceInfo* service);
	Destination* removeDestination(const char * aServiceName);
	void removeAdminDestination(char* svcname);

	ConnectionManager connections;
	std::vector<ServiceData> serviceData;
	std::vector<char*> advertisedServices;
	char* serverName;
	//ServerMetadata serverInfo;
	ServerInfo serverInfo;
	PortableServer::POA_var poa;

	// The following are not implemented
	//
	AtmiBrokerServer(const AtmiBrokerServer &);
	AtmiBrokerServer& operator=(const AtmiBrokerServer &);
};

// SERVER
extern BLACKTIE_XATMI_DLL AtmiBrokerServer * ptrServer;

#endif
