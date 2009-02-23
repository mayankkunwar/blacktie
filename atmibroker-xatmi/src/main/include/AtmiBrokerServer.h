/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat Middleware LLC, and others contributors as indicated
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
/*
 * BREAKTHRUIT PROPRIETARY - NOT TO BE DISCLOSED OUTSIDE BREAKTHRUIT, LLC.
 */
// copyright 2006, 2008 BreakThruIT

//-----------------------------------------------------------------------------
// Edit the idlgen.cfg to have your own copyright notice placed here.
//-----------------------------------------------------------------------------

// Class: AtmiBrokerServer
// A POA servant which implements of the AtmiBroker::Server interface
//


#ifndef ATMIBROKER_SERVERSERVERIMPL_H_
#define ATMIBROKER_SERVERSERVERIMPL_H_

#include <iostream>
#include <vector>

#ifdef TAO_COMP
#include "AtmiBrokerS.h"
#endif
#include "CorbaConnection.h"
#include "ConnectionImpl.h"

#include "xatmi.h"
#include "AtmiBrokerServerXml.h"
#include "AtmiBrokerServiceXml.h"
#include "Destination.h"
#include "ServiceDispatcher.h"

struct _service_data {
	Destination* destination;
	void (*func)(TPSVCINFO *);
	std::vector<ServiceDispatcher*> dispatchers;
	SVCINFO serviceInfo;
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

	bool isAdvertised(char * serviceName);
	int block();

private:
	ConnectionImpl* serverConnection;
	CORBA_CONNECTION* realConnection;
	void (*getServiceMethod(const char * aServiceName))(TPSVCINFO *);
	void addDestination(Destination* destination, void(*func)(TPSVCINFO *));
	Destination* removeDestination(const char * aServiceName);

	std::vector<ServiceData> serviceData;
	std::vector<char*> advertisedServices;
	char * serverName;
	ServerMetadata serverInfo;
	PortableServer::POA_ptr poa;

	// The following are not implemented
	//
	AtmiBrokerServer(const AtmiBrokerServer &);
	AtmiBrokerServer& operator=(const AtmiBrokerServer &);
};

// SERVER
extern AtmiBrokerServer * ptrServer;

#endif
