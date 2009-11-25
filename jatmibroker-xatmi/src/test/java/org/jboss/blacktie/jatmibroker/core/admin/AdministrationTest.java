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
package org.jboss.blacktie.jatmibroker.core.admin;

import java.util.Properties;

import javax.management.MBeanServerConnection;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

import junit.framework.TestCase;

import org.jboss.blacktie.jatmibroker.core.conf.XMLEnvHandler;
import org.jboss.blacktie.jatmibroker.core.conf.XMLParser;
import org.w3c.dom.Element;

public class AdministrationTest extends TestCase {
	public void testTODO() {

	}

	public void xtest() throws Exception {
		Properties prop = new Properties();
		XMLEnvHandler handler = new XMLEnvHandler("", prop);
		XMLParser xmlenv = new XMLParser(handler, "Environment.xsd");
		xmlenv.parse("Environment.xml");

		JMXServiceURL u = new JMXServiceURL((String) prop.get("JMXURL"));
		JMXConnector c = JMXConnectorFactory.connect(u);
		MBeanServerConnection beanServerConnection = c
				.getMBeanServerConnection();

		String server = "foo";
		String service = "BAR";
		AdministrationProxy serverAdministration = new AdministrationProxy(
				beanServerConnection);
		Element aServerInfo = serverAdministration.getServersStatus();
		long serviceCounter = serverAdministration.getServiceCounter(server,
				service);
		serverAdministration.unadvertise(server, service);
		serverAdministration.advertise(server, service);
		serverAdministration.shutdown(server, 0);

		serverAdministration.close();
		c.close();
	}
}
