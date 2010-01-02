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
#include "AtmiBrokerServerControl.h"
#include "ConnectionManager.h"
#include "Destination.h"
#include "ServiceDispatcher.h"
#include "SynchronizableObject.h"

struct _service_data {
	Destination* destination;
	void (*func)(TPSVCINFO *);
	std::vector<ServiceDispatcher*> dispatchers;
	ServiceInfo* serviceInfo;
};
typedef _service_data ServiceData;
class AtmiBrokerServer {
public:
	AtmiBrokerServer();

	virtual ~AtmiBrokerServer();

	virtual char * getServerName();

	virtual bool advertiseService(char * serviceName, void(*func)(TPSVCINFO *));

	virtual void unadvertiseService(char * serviceName);

	BLACKTIE_XATMI_DLL bool isAdvertised(char * serviceName);
	bool advertiseService(char* serviceName);
	void advertiseAtBootime();
	int  getServiceStatus(char** str, char* svc);
	long getServiceMessageCounter(char* serviceName);
	void server_done();
	int  block();
	void shutdown();
	int  pause();
	int  resume();

private:
	void (*getServiceMethod(const char * aServiceName))(TPSVCINFO *);
	void addDestination(Destination* destination, void(*func)(TPSVCINFO *), ServiceInfo* service);
	Destination* removeDestination(const char * aServiceName);
	bool createAdminDestination(char* svcname);
	void removeAdminDestination(char* svcname);
	void updateServiceStatus(ServiceInfo* service, SVCFUNC func, bool status);

	ConnectionManager connections;
	std::vector<ServiceData> serviceData;
	std::vector<ServiceStatus> serviceStatus;
	std::vector<char*> advertisedServices;
	char* serverName;
	ServerInfo serverInfo;
	SynchronizableObject* finish;
	bool isPause;

	// The following are not implemented
	//
	AtmiBrokerServer(const AtmiBrokerServer &);
	AtmiBrokerServer& operator=(const AtmiBrokerServer &);
};

// SERVER
extern BLACKTIE_XATMI_DLL AtmiBrokerServer * ptrServer;

#endif
