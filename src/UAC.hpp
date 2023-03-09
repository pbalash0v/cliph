#ifndef basic_invite_UAC_hpp
#define basic_invite_UAC_hpp

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

#include <sstream>
#include <atomic>
#include <time.h>
#include <utility>


namespace
{

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

// Generic InviteSessionHandler
class InviteSessionHandler : public resip::InviteSessionHandler, public resip::OutOfDialogHandler
{
public:
	resip::Data name;

	InviteSessionHandler(const resip::Data& n)
		: name(n)
	{
	}

	void onNewSession(resip::ClientInviteSessionHandle, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ClientInviteSession-onNewSession - " << msg.brief() << std::endl;
	}

	void onNewSession(resip::ServerInviteSessionHandle, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ServerInviteSession-onNewSession - " << msg.brief() << std::endl;
	}

	void onFailure(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ClientInviteSession-onFailure - " << msg.brief() << std::endl;

		const resip::StatusLine& sLine = msg.header(resip::h_StatusLine);
		assert(sLine.responseCode() != 500);
	}

	void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ClientInviteSession-onProvisional - " << msg.brief() << std::endl;
	}

	void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ClientInviteSession-onConnected - " << msg.brief() << std::endl;
	}

	void onStaleCallTimeout(resip::ClientInviteSessionHandle) override
	{
		std::cout << name << ": ClientInviteSession-onStaleCallTimeout" << std::endl;
	}

	void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onConnected - " << msg.brief() << std::endl;
	}

	void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ClientInviteSession-onRedirected - " << msg.brief() << std::endl;
	}

	void onTerminated(resip::InviteSessionHandle, resip::InviteSessionHandler::TerminatedReason, const resip::SipMessage* msg) override
	{
		std::cout << name << ": InviteSession-onTerminated - " << msg->brief() << std::endl;
		assert(false); // This is overriden in UAS and UAC specific handlers
	}

	void onAnswer(resip::InviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override
	{
		std::cout << name << ": InviteSession-onAnswer(SDP)" << std::endl;
		//sdp->encode(std::cout);
	}

	void onOffer(resip::InviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override
	{
		std::cout << name << ": InviteSession-onOffer(SDP)" << std::endl;
		//sdp->encode(std::cout);
	}

	void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override
	{
		std::cout << name << ": InviteSession-onEarlyMedia(SDP)" << std::endl;
		//sdp->encode(std::cout);
	}

	void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onOfferRequired - " << msg.brief() << std::endl;
	}

	void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage*) override
	{
		std::cout << name << ": InviteSession-onOfferRejected" << std::endl;
	}

	void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onRefer - " << msg.brief() << std::endl;
	}

	void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onReferAccepted - " << msg.brief() << std::endl;
	}

	void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onReferRejected - " << msg.brief() << std::endl;
	}

	void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onReferNoSub - " << msg.brief() << std::endl;
	}

	void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onInfo - " << msg.brief() << std::endl;
	}

	void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onInfoSuccess - " << msg.brief() << std::endl;
	}

	void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onInfoFailure - " << msg.brief() << std::endl;
	}

	void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onMessage - " << msg.brief() << std::endl;
	}

	void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onMessageSuccess - " << msg.brief() << std::endl;
	}

	void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override
	{
		std::cout << name << ": InviteSession-onMessageFailure - " << msg.brief() << std::endl;
	}

	void onForkDestroyed(resip::ClientInviteSessionHandle) override
	{
		std::cout << name << ": ClientInviteSession-onForkDestroyed" << std::endl;
	}

	// Out-of-Dialog Callbacks
	void onSuccess(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& successResponse) override
	{
		std::cout << name << ": ClientOutOfDialogReq-onSuccess - " << successResponse.brief() << std::endl;
	}
	void onFailure(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& errorResponse) override
	{
		std::cout << name << ": ClientOutOfDialogReq-onFailure - " << errorResponse.brief() << std::endl;
	}
	void onReceivedRequest(resip::ServerOutOfDialogReqHandle ood, const resip::SipMessage& request) override
	{
		std::cout << name << ": ServerOutOfDialogReq-onReceivedRequest - " << request.brief() << std::endl;
		// Add SDP to response here if required
		std::cout << name << ": Sending 200 response to OPTIONS." << std::endl;
		ood->send(ood->answerOptions());
	}
}; //class InviteSessionHandler

class TestUac : public InviteSessionHandler
{
public:
	bool done {false};

	std::unique_ptr<resip::SdpContents> mSdp;
	std::unique_ptr<resip::HeaderFieldValue> hfv;
	std::unique_ptr<resip::Data> txt;

	TestUac()
		: InviteSessionHandler("UAC")
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

	// handle incoming INVITE (i.e. sip_sig is caller)
	void onNewSession(resip::ServerInviteSessionHandle sis_h, resip::InviteSession::OfferAnswerType, const resip::SipMessage& msg) override
	{
		std::cout << name << ": ServerInviteSession-onNewSession - " << msg.brief() << std::endl;
		sis_h->reject(403);
	}

	void onOffer(resip::InviteSessionHandle is, const resip::SipMessage&, const resip::SdpContents& sdp) override
	{
		std::cout << name << ": InviteSession-onOffer(SDP)" << std::endl;
		//sdp->encode(std::cout);
		is->provideAnswer(sdp);
	}

	void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage&) override
	{
		//BOOST_ASSERT(false);
	}

	void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage&) override
	{
		assert(false);
	}

	void onInfo(resip::InviteSessionHandle, const resip::SipMessage&) override
	{
		assert(false);
	}

	void onTerminated(resip::InviteSessionHandle, resip::InviteSessionHandler::TerminatedReason, const resip::SipMessage* msg) override
	{
		std::cout << name << ": InviteSession-onTerminated - ";
		if (msg)
		{
			std::cout << msg->brief() << std::endl;
		}
		std::cout << std::endl;

		done = true;
	}
};

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


void run_UAC(std::atomic_bool& should_stop, const std::string& username, resip::TransportType transport, uint16_t port = 0)
{
	resip::Log::initialize(resip::Log::Cout, resip::Log::Info, resip::Data{"cliphone"});

	auto doReg = false;
	resip::Data uacPasswd;
	resip::Data uasPasswd;
	auto useOutbound = false;
	resip::Uri outboundUri;

	auto uacAor = resip::NameAddr("sip:caller@127.0.0.1");
	auto uasAor = resip::NameAddr(resip::Data{std::string {"sip:"} + username + std::string{"@127.0.0.1"}});

	//set up UAC
	auto stackUac = resip::SipStack{};
	stackUac.addTransport(transport, port);

	auto dum = std::make_unique<resip::DialogUsageManager>(stackUac);
	dum->setMasterProfile(std::make_shared<resip::MasterProfile>());
	dum->setClientAuthManager(std::make_unique<resip::ClientAuthManager>());

	auto uac = TestUac{};
	dum->setInviteSessionHandler(&uac);
	dum->addOutOfDialogHandler(resip::OPTIONS, &uac);

	dum->setAppDialogSetFactory(std::make_unique<resip::AppDialogSetFactory>());

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

	bool startedCallFlow = false;
	bool startedShutdown = false;
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
			startedCallFlow = true;
			dum->send(dum->makeInviteSession(uasAor, uac.mSdp.get()));
		}

		if ((should_stop || uac.done) && !startedShutdown)
		{
			dum->shutdown(&uacShutdownHandler);
			startedShutdown = true;
		}
	}
}

} // namespace

#endif // basic_invite_UAC_hpp