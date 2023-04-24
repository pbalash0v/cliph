#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iostream>
#include <utility>
//
#include <time.h>
//
#include "controller.hpp"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
//
#include "sip.hpp"
#include "sdp.hpp"

namespace
{
struct DUMShutdownHandler : public resip::DumShutdownHandler
{
	void onDumCanBeDeleted() override
	{
		std::cout << "onDumCanBeDeleted." << std::endl;
		dumShutDown = true;
	}

	resip::Data name;
	bool dumShutDown{false};	
}; //struct DUMShutdownHandler


class sip_agent : public resip::InviteSessionHandler, public resip::OutOfDialogHandler
{
public:	
	bool done {false};
	resip::SdpContents mSdp;
	cliph::sip::callback_type m_cback;

public:
	sip_agent(std::string loc_sd, cliph::sip::callback_type&& cback)
		: m_cback{cback}
	{
		auto hfv = resip::HeaderFieldValue{loc_sd.c_str(), (unsigned int)loc_sd.size()};
		auto type = resip::Mime{"application", "sdp"};
		mSdp = resip::SdpContents{hfv, type};
	}

	void shutdown()
	{
		done = true;
	}

public:
	void onNewSession(resip::ClientInviteSessionHandle, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << "ClientInviteSession-onNewSession - " << msg.brief() << std::endl;
	}

	// handle incoming INVITE
	void onNewSession(resip::ServerInviteSessionHandle sis_h, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << "ServerInviteSession-onNewSession - " << msg.brief() << std::endl;
		sis_h->reject(403);
	}

	void onFailure(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "ClientInviteSession-onFailure - " << msg.brief() << std::endl;

		const resip::StatusLine& sLine = msg.header(resip::h_StatusLine);
		assert(sLine.responseCode() != 500);
	}

	void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "ClientInviteSession-onProvisional - " << msg.brief() << std::endl;
	}

	void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "ClientInviteSession-onConnected - " << msg.brief() << std::endl;
		m_cback({cliph::sip::event::kind::on_connected});
	}

	void onStaleCallTimeout(resip::ClientInviteSessionHandle) override
	{
		std::cout << "ClientInviteSession-onStaleCallTimeout" << std::endl;
	}

	void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onConnected - " << msg.brief() << std::endl;
	}

	void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "ClientInviteSession-onRedirected - " << msg.brief() << std::endl;
	}

	void onTerminated(resip::InviteSessionHandle, resip::InviteSessionHandler::TerminatedReason, const resip::SipMessage* msg) override
	{
		std::cout << "InviteSession-onTerminated - ";
		if (msg)
		{
			std::cout << msg->brief() << std::endl;
		}
		std::cout << std::endl;
		m_cback({cliph::sip::event::kind::on_hangup});
		shutdown();
	}

	void onAnswer(resip::InviteSessionHandle is, const resip::SipMessage&, const resip::SdpContents& sdp) override
	{
		std::cout << "InviteSession-onAnswer(SDP)" << std::endl;
		sdp.encode(std::cout);

		const auto hangup = [&](const auto& what)
		{
			std::cout << what << std::endl;
			is->end();
			m_cback({cliph::sip::event::kind::on_hangup});
		};

		if (sdp.session().media().front().name() != "audio")
		{
			hangup("First media section is not audio");
			return;
		}

		auto list = sdp.session().media().front().getConnections();
		if (list.empty()) // TODO: look up connection in sdp globals
		{
			hangup("Front media section has no connection");
			return;
		}

		for (const auto& codec : sdp.session().media().front().codecs())
		{
			if (codec.getName().prefix("OPUS") or (codec.getName().prefix("opus")))
			{
				auto ept = cliph::sip::event::media_ept_type{list.front().getAddress().c_str(), sdp.session().media().front().port()};
				auto opus_pt = static_cast<uint8_t>(codec.payloadType());
				auto ret = cliph::sip::event{cliph::sip::event::kind::on_answer, std::move(ept), opus_pt};
				m_cback(std::move(ret));
				return;
			}
		}

		hangup("No OPUS codec in answer SDP");
	}

	void onOffer(resip::InviteSessionHandle is, const resip::SipMessage&, const resip::SdpContents& sdp) override
	{
		std::cout << "InviteSession-onOffer(SDP)" << std::endl;
		sdp.encode(std::cout);
		is->reject(488u);
	}

	void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents& sdp) override
	{
		std::cout << "InviteSession-onEarlyMedia(SDP)" << std::endl;
		sdp.encode(std::cout);
	}

	void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onOfferRequired - " << msg.brief() << std::endl;
	}

	void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage*) override
	{
		std::cout << "InviteSession-onOfferRejected" << std::endl;
	}

	void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onRefer - " << msg.brief() << std::endl;
	}

	void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onReferAccepted - " << msg.brief() << std::endl;
	}

	void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onReferRejected - " << msg.brief() << std::endl;
	}

	void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onReferNoSub - " << msg.brief() << std::endl;
	}

	void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onInfo - " << msg.brief() << std::endl;
	}

	void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onInfoSuccess - " << msg.brief() << std::endl;
	}

	void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onInfoFailure - " << msg.brief() << std::endl;
	}

	void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onMessage - " << msg.brief() << std::endl;
	}

	void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onMessageSuccess - " << msg.brief() << std::endl;
	}

	void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << "InviteSession-onMessageFailure - " << msg.brief() << std::endl;
	}

	void onForkDestroyed(resip::ClientInviteSessionHandle) override
	{
		std::cout << "ClientInviteSession-onForkDestroyed" << std::endl;
	}

	// Out-of-Dialog Callbacks
	void onSuccess(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& successResponse) override
	{
		std::cout << "ClientOutOfDialogReq-onSuccess - " << successResponse.brief() << std::endl;
	}
	void onFailure(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& errorResponse) override
	{
		std::cout << "ClientOutOfDialogReq-onFailure - " << errorResponse.brief() << std::endl;
	}
	void onReceivedRequest(resip::ServerOutOfDialogReqHandle ood, const resip::SipMessage& request) override
	{
		std::cout << "ServerOutOfDialogReq-onReceivedRequest - " << request.brief() << std::endl;
		// Add SDP to response here if required
		std::cout << "Sending 200 response to OPTIONS." << std::endl;
		ood->send(ood->answerOptions());
	}
}; //class sip_agent

} // namespace


namespace cliph::sip
{
ua::ua(const config& cfg)
	: m_config{cfg}
{
}

void ua::stop() noexcept
{
	m_should_stop = true;
}

ua::~ua()
{
	stop();
    if (m_agent_thr.joinable())
    {
		m_agent_thr.join();
    }
}

void ua::call(std::string local_sd)
{
    m_agent_thr = std::thread{[&, local_sd=std::move(local_sd)]
    {
		resip::Log::initialize(resip::Log::Cout, (m_config.verbose ? resip::Log::Debug : resip::Log::Info), resip::Data{"cliph"});
		//
		auto myAor = resip::NameAddr{resip::Data{m_config.from.c_str()}};
		auto calleeAor = resip::NameAddr{resip::Data{m_config.to.c_str()}};
		//
		//set up UAC
		//
		auto stackUac = resip::SipStack{};
		stackUac.addTransport(m_config.tp_cfg.transport, m_config.tp_cfg.port);
		// DUM
		auto dum = std::make_unique<resip::DialogUsageManager>(stackUac);
		// Profile
		auto profile = std::make_shared<resip::MasterProfile>();
		profile->setUserAgent("cliph");
		profile->setDigestCredential(myAor.uri().host(), m_config.auth.c_str(), m_config.pswd.c_str());
		if (m_config.outbound_prx)
		{
			profile->setOutboundProxy(resip::Uri{m_config.outbound_prx->c_str()});
			profile->addSupportedOptionTag(resip::Token(resip::Symbols::Outbound));
		}
		profile->setDefaultFrom(myAor);
		dum->setMasterProfile(std::move(profile));
		//
		dum->setClientAuthManager(std::make_unique<resip::ClientAuthManager>());
		//
		auto agent = sip_agent{local_sd, std::move(m_config.cback)};
		dum->setInviteSessionHandler(&agent);
		dum->addOutOfDialogHandler(resip::OPTIONS, &agent);
		//
		dum->setAppDialogSetFactory(std::make_unique<resip::AppDialogSetFactory>());//app_dialog_set_factory>());
		//
		auto uacShutdownHandler = DUMShutdownHandler{};

		// Call loop
		bool startedCallFlow = false;
		bool startedShutdown = false;
		auto inv = std::shared_ptr<resip::SipMessage>{};
		while (not uacShutdownHandler.dumShutDown)
		{
			if (not uacShutdownHandler.dumShutDown)
			{
				stackUac.process(50);
				while (dum->process())
				{
				}
			}

			if (not startedCallFlow)
			{
				inv = dum->makeInviteSession(calleeAor, &agent.mSdp);
				dum->send(inv);
				startedCallFlow = true;
			}

			if (m_should_stop || agent.done)
			{
				if (not startedShutdown)
				{
					if (auto adsh = dum->findAppDialogSet(resip::DialogSetId{*inv}); adsh.isValid())
					{
						adsh->end();
					}
					dum->shutdown(&uacShutdownHandler);
					startedShutdown = true;				
				}
			}
		}
	}};
}


} // namespace cliph:sip

