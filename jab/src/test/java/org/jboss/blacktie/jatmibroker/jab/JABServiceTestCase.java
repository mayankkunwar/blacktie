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
package org.jboss.blacktie.jatmibroker.jab;

import junit.framework.TestCase;

public class JABServiceTestCase extends TestCase {
	private RunServer runServer = new RunServer();

	public void setUp() throws InterruptedException {
		runServer.serverinit();
	}

	public void tearDown() {
		runServer.serverdone();
	}

	public void testJABService() throws Exception {
		String domainName = "fooapp";
		String serverName = "foo";
		String serviceName = "BAR";
		String transactionManagerService = "TransactionManagerService.OTS";
		String[] args = new String[2];
		args[0] = "-ORBInitRef";
		args[1] = "NameService=corbaloc::localhost:3528/NameService";

		JABSessionAttributes aJabSessionAttributes = new JABSessionAttributes(domainName, serverName, transactionManagerService, args);
		JABSession aJabSession = new JABSession(aJabSessionAttributes);
		JABRemoteService aJabService = new JABRemoteService(aJabSession, serviceName);
		aJabService.setString("STRING", "HOWS IT GOING DUDE????!!!!");
		aJabService.call(null);
		aJabSession.endSession();
		String expectedString = "BAR SAYS HELLO";
		String responseString = aJabService.getResponseString();
		assertEquals(expectedString, responseString);
	}

}
