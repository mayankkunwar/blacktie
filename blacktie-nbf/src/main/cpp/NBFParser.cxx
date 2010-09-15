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
#include <log4cxx/logger.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <NBFParser.h>

log4cxx::LoggerPtr NBFParser::logger(log4cxx::Logger::getLogger("NBFParser"));

NBFParser::NBFParser() {
	try {
		XMLPlatformUtils::Initialize();
		isInitial = true;
		SAXParser* parser = new SAXParser;
		parser->setValidationScheme(SAXParser::Val_Auto);
		parser->setDoNamespaces(true);
		parser->setDoSchema(true);
		parser->setValidationSchemaFullChecking(true);
		LOG4CXX_DEBUG(logger, "create SAXParser");
	} catch (const XMLException& toCatch){
		LOG4CXX_ERROR(logger, "Error during initialization! Message:"
				<< StrX(toCatch.getMessage()));
		isInitial = false;
		parser = NULL;
	}
}

NBFParser::~NBFParser() {
	XMLPlatformUtils::Terminate();
	if(parser != NULL) {
		delete parser;
		LOG4CXX_DEBUG(logger, "release parser");
	}
}

bool NBFParser::parse(const char* buf) {
	return false;
}
