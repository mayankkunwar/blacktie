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
#include <cppunit/extensions/HelperMacros.h>

#include "TestAtmiBrokerXml.h"
#include "AtmiBrokerServiceXml.h"
#include "AtmiBrokerEnvXml.h"
#include "AtmiBrokerEnv.h"
#include "ace/OS_NS_stdlib.h"

void TestAtmiBrokerXml::test_service() {
	AtmiBrokerServiceXml xml;
	ServiceInfo service;

	xml.parseXmlDescriptor(&service, "BAR");
	CPPUNIT_ASSERT(service.poolSize == 5);
	CPPUNIT_ASSERT(strcmp(service.function_name, "BAR") == 0);
	CPPUNIT_ASSERT(strcmp(service.library_name, "libBAR.so") == 0);
	CPPUNIT_ASSERT(service.advertised == false);

	free(service.function_name);
	free(service.library_name);
}

void TestAtmiBrokerXml::test_env() {
	char* value;

	value = AtmiBrokerEnv::get_instance()->getenv((char*)"ORBOPT");
	CPPUNIT_ASSERT(strcmp(value, "-ORBInitRef NameService=corbaloc::localhost:3528/NameService") == 0);
	CPPUNIT_ASSERT(strcmp(domain, "fooapp") == 0);
	CPPUNIT_ASSERT(xarmp != 0);

	CPPUNIT_ASSERT(servers.size() == 2);
	ServerInfo* server = servers[1];
	CPPUNIT_ASSERT(server != NULL);
	CPPUNIT_ASSERT(strcmp(server->serverName, "foo") == 0);
	std::vector<ServiceInfo>* services = &server->serviceVector;
	CPPUNIT_ASSERT(strcmp((*services)[0].serviceName, "BAR") == 0);
	CPPUNIT_ASSERT((*services)[0].poolSize == 5);
	CPPUNIT_ASSERT(strcmp((*services)[0].function_name, "BAR") == 0);
#ifdef WIN32
	CPPUNIT_ASSERT(strcmp((*services)[0].transportLib, "atmibroker-corba.dll") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[0].library_name, "BAR.dll") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[1].serviceName, "ECHO") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[1].transportLib, "atmibroker-stomp.dll") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[2].serviceName, "foo_ADMIN") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[2].transportLib, "atmibroker-stomp.dll") == 0);
#else
	CPPUNIT_ASSERT(strcmp((*services)[0].transportLib, "libatmibroker-corba.so") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[0].library_name, "libBAR.so") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[1].serviceName, "ECHO") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[1].transportLib, "libatmibroker-stomp.so") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[2].serviceName, "foo_ADMIN") == 0);
	CPPUNIT_ASSERT(strcmp((*services)[2].transportLib, "libatmibroker-stomp.so") == 0);
#endif
	CPPUNIT_ASSERT((*services)[0].advertised == false);


	char* transport = AtmiBrokerEnv::get_instance()->getTransportLibrary((char*)"BAR");
#ifdef WIN32
	CPPUNIT_ASSERT(strcmp(transport, "atmibroker-corba.dll") == 0);
#else
	CPPUNIT_ASSERT(strcmp(transport, "libatmibroker-corba.so") == 0);
#endif

	AtmiBrokerEnv::discard_instance();
}
