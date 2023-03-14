#include <memory>
#include <sstream>
#include <iostream>
#include <utility>
//
#include <time.h>
//
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
#include "sip_agent.hpp"

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


#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class sip_agent : public resip::InviteSessionHandler, public resip::OutOfDialogHandler
{
public:
	bool done {false};

	std::unique_ptr<resip::SdpContents> mSdp;
	std::unique_ptr<resip::HeaderFieldValue> hfv;
	std::unique_ptr<resip::Data> txt;

public:
	sip_agent()
	{
		txt = std::make_unique<resip::Data>("v=0\r\n"
											"o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
											"s=X-Lite\r\n"
											"c=IN IP4 192.168.2.15\r\n"
											"t=0 0\r\n"
											"m=audio 8000 RTP/AVP 8 3 98 97 101\r\n"
											"a=rtpmap:8 pcma/8000\r\n"
											"a=rtpmap:3 gsm/8000\r\n"
											"a=rtpmap:98 iLBC\r\n"
											"a=rtpmap:97 speex/8000\r\n"
											"a=rtpmap:101 telephone-event/8000\r\n"
											"a=fmtp:101 0-15\r\n");

		hfv = std::make_unique<resip::HeaderFieldValue>(txt->data(), (unsigned int)txt->size());
		resip::Mime type("application", "sdp");
		mSdp = std::make_unique<resip::SdpContents>(*hfv, type);
	}

public:
	void onNewSession(resip::ClientInviteSessionHandle, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << "\n\n\n\nClientInviteSession-onNewSession - " << msg.brief() << std::endl;
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

		done = true;
	}

	void onAnswer(resip::InviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override
	{
		std::cout << "InviteSession-onAnswer(SDP)" << std::endl;
		//sdp->encode(std::cout);
	}

	void onOffer(resip::InviteSessionHandle is, const resip::SipMessage&, const resip::SdpContents& sdp) override
	{
		std::cout << "InviteSession-onOffer(SDP)" << std::endl;
		//sdp->encode(std::cout);
		is->provideAnswer(sdp);
	}

	void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override
	{
		std::cout << "InviteSession-onEarlyMedia(SDP)" << std::endl;
		//sdp->encode(std::cout);
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


class app_dialog_set : public resip::AppDialogSet
{
public:
	app_dialog_set(resip::DialogUsageManager& dum)
		: resip::AppDialogSet{dum}
		, mDum{dum}
	{
	}

	resip::AppDialog* createAppDialog(const resip::SipMessage&) override
	{
		return std::make_unique<resip::AppDialog>(mDum).release();
	}

	resip::DialogUsageManager& mDum;
}; // class app_dialog_set

#if 0
class app_dialog_set_factory : public resip::AppDialogSetFactory
{
public:
	resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager& dum, const resip::SipMessage&)
	{
		return std::make_unique<app_dialog_set>(dum).release();
	}

}; // class app_dialog_set_factory
#endif
} // namespace



namespace cliph::sip
{
agent& agent::get() noexcept
{
	static auto instance = agent{};
	return instance;
}


void agent::stop() noexcept
{
	m_should_stop = true;
}

agent::~agent()
{
	stop();
    if (m_agent_thread.joinable())
    {
		m_agent_thread.join();
    }
}

void agent::run(on_answer_cback_type&& on_ans_cb, on_hangup_cback_type&& on_hang_cb)
{
	const auto loop = [&, on_ans_cb=std::move(on_ans_cb), on_hang_cb=std::move(on_hang_cb)]()
	{
		resip::Log::initialize(resip::Log::Cout, resip::Log::Stack, resip::Data{"cliphone"});

		auto doReg = false;
		resip::Data uacPasswd;
		resip::Data uasPasswd;
		auto useOutbound = false;
		resip::Uri outboundUri;

		auto uacAor = resip::NameAddr{resip::Data{"sip:"} + m_call_config.from_user.c_str() + "@" + m_call_config.from_domain.c_str()};
		auto uasAor = resip::NameAddr{resip::Data{"sip:"} + m_call_config.to_user.c_str() + "@" + m_call_config.to_domain.c_str()};

		//set up UAC
		auto stackUac = resip::SipStack{};
		stackUac.addTransport(m_config.transport, m_config.port);

		auto dum = std::make_unique<resip::DialogUsageManager>(stackUac);
		dum->setMasterProfile(std::make_shared<resip::MasterProfile>());
		dum->setClientAuthManager(std::make_unique<resip::ClientAuthManager>());

		auto agent = sip_agent{};
		dum->setInviteSessionHandler(&agent);
		dum->addOutOfDialogHandler(resip::OPTIONS, &agent);

		dum->setAppDialogSetFactory(std::make_unique<resip::AppDialogSetFactory>());//app_dialog_set_factory>());

		if (doReg)
		{
			dum->getMasterProfile()->setDigestCredential(uacAor.uri().host(), uacAor.uri().user(), uacPasswd);
		}

		if (useOutbound)
		{
			dum->getMasterProfile()->setOutboundProxy(outboundUri);
			dum->getMasterProfile()->addSupportedOptionTag(resip::Token(resip::Symbols::Outbound));
		}

		dum->getMasterProfile()->setDefaultFrom(uacAor);

		auto uacShutdownHandler = DUMShutdownHandler{};

		//
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
				inv = dum->makeInviteSession(uasAor, agent.mSdp.get());//, ads.release());
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
	};

    m_agent_thread = std::thread{loop};
}

} // namespace cliph:sip

