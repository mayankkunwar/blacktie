package org.jboss.blacktie.jatmibroker.core;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.blacktie.jatmibroker.core.jab.JABTransaction;
import org.jboss.blacktie.jatmibroker.core.jab.ThreadActionData;
import org.omg.CORBA.BAD_PARAM;
import org.omg.CORBA.LocalObject;
import org.omg.CORBA.SystemException;
import org.omg.CosTransactions.Control;
import org.omg.IOP.Codec;
import org.omg.IOP.TaggedComponent;
import org.omg.IOP.ServiceContext;
import org.omg.PortableInterceptor.ClientRequestInfo;
import org.omg.PortableInterceptor.ClientRequestInterceptor;
import org.omg.PortableInterceptor.ServerRequestInfo;
import org.omg.PortableInterceptor.ServerRequestInterceptor;

public class TxRequestInterceptor extends LocalObject implements
		ClientRequestInterceptor, ServerRequestInterceptor {
	private static final Logger log = LogManager.getLogger(TxRequestInterceptor.class);

	// ORB request service context id for tagging service contexts (as
	// transactional)
	// must match up with C++ implementation (see TxInterceptor.h)
	private static final int tx_context_id = 0xabcd;

	private Codec codec;
	private String name;

	public TxRequestInterceptor(String name, Codec codec) {
		this.codec = codec;
		this.name = name;
	}

	//
	// Interceptor operations
	//

	public String name() {
		return name;
	}

	public void destroy() {
	}

	private String extract_ior_from_context(ServerRequestInfo ri) {
		return null;
	}

	private String extract_ior_from_context(ClientRequestInfo ri) {
		return null;
	}

	public void send_request(ClientRequestInfo ri) {
		if (isTransactional(ri)) {
			log.trace("TxRequestInterceptor.send_request: " + ri.operation());
			JABTransaction curr = ThreadActionData.currentAction();

			if (curr != null) {
				log.trace("adding tx");
				Control control = curr.getControl();
				String ior = AtmiBrokerServerProxy.orbManagement.getOrb()
						.object_to_string(control);
				log.trace("tx ior: " + ior);
				ri.add_request_service_context(new ServiceContext(tx_context_id,
						ior.getBytes()), false);
			}
		}
	}

	public void send_poll(ClientRequestInfo ri) {
		log.trace("TxRequestInterceptor.send_poll: " + ri.operation());
	}

	public void receive_reply(ClientRequestInfo ri) {
		log.trace("TxRequestInterceptor.receive_reply: " + ri.operation());
	}

	public void receive_exception(ClientRequestInfo ri) {
		log.trace("TxRequestInterceptor.receive_exception: " + ri.operation());
	}

	public void receive_other(ClientRequestInfo ri) {
		log.trace("TxRequestInterceptor.receive_other: " + ri.operation());
	}

	//
	// ServerRequestInterceptor operations
	//

	public void receive_request_service_contexts(ServerRequestInfo ri) {
		log.trace("TxRequestInterceptor.receive_request_service_contexts: " + ri.operation());

		try {
			ServiceContext serviceContext = ri
					.get_request_service_context(tx_context_id);
			byte[] data = serviceContext.context_data;
			log.trace("TxRequestInterceptor.receive_request_service_contexts: data: "
							+ data);
		} catch (BAD_PARAM e) {
			log.debug("TxRequestInterceptor.receive_request_service_contexts: not transactional");
			// not a transactional request - ignore
		} catch (SystemException e) {
			log.error("receive_request_service_contexts error: ", e);
		}
	}

	public void receive_request(ServerRequestInfo ri) {
		log.trace("TxRequestInterceptor.receive_request: " + ri.operation());
	}

	public void send_reply(ServerRequestInfo ri) {
		log.trace("TxRequestInterceptor.send_reply: " + ri.operation());
	}

	public void send_exception(ServerRequestInfo ri) {
		log.trace("TxRequestInterceptor.send_exception " + ri.operation() + " exception: " + ri.sending_exception());
	}

	public void send_other(ServerRequestInfo ri) {
		log.trace("TxRequestInterceptor.send_other: " + ri.operation());
	}

	private boolean isTransactional(ClientRequestInfo ri) {
		try {
			TaggedComponent comp = ri.get_effective_component(TxIORInterceptor.TAG_OTS_POLICY);
			log.debug("isTransactional: " + ri.operation ());

			return true;
		} catch (SystemException e) {
			// should be BAD_PARAM, minor code 25
		} catch (Throwable t) {
			log.warn("isTransactional: unexpected ex: " + t.getMessage());
		}

		return false;
	}
}
