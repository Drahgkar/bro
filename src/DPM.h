// The central management unit for dynamic analyzer selection.

#ifndef DPM_H
#define DPM_H

#include <queue>

#include "AnalyzerTags.h"
#include "Dict.h"
#include "IPAddr.h"
#include "net_util.h"

struct AnalyzerConfig;
class Analyzer;
class Connection;

typedef bool (*analyzer_config_available_callback)(const AnalyzerTag& tag);
typedef Analyzer* (*analyzer_config_factory_callback)(Connection* conn, const AnalyzerTag& tag);
typedef bool (*analyzer_config_match_callback)(Connection*);

// DPM debug logging, which includes the connection id into the message.
#ifdef DEBUG
# define DBG_DPD(conn, txt) \
	DBG_LOG(DBG_DPD, "%s " txt, \
		fmt_conn_id(conn->OrigAddr(), ntohs(conn->OrigPort()), \
		conn->RespAddr(), ntohs(conn->RespPort())));
# define DBG_DPD_ARGS(conn, fmt, args...) \
	DBG_LOG(DBG_DPD, "%s " fmt, \
		fmt_conn_id(conn->OrigAddr(), ntohs(conn->OrigPort()), \
		conn->RespAddr(), ntohs(conn->RespPort())), ##args);
#else
# define DBG_DPD(conn, txt)
# define DBG_DPD_ARGS(conn, fmt, args...)
#endif

// Map to assign expected connections to analyzers.
class ExpectedConn {
public:
	ExpectedConn(const IPAddr& _orig, const IPAddr& _resp,
			uint16 _resp_p, uint16 _proto);

	ExpectedConn(const ExpectedConn& c);

	IPAddr orig;
	IPAddr resp;
	uint16 resp_p;
	uint16 proto;
};

// Associates an analyzer for an expected future connection.
class AssignedAnalyzer {
public:
	AssignedAnalyzer(const ExpectedConn& c)
	: conn(c)
		{
		}

	ExpectedConn conn;
	AnalyzerTag analyzer;
	double timeout;
	void* cookie;
	bool deleted;

	static bool compare(const AssignedAnalyzer* a1, const AssignedAnalyzer* a2)
		{ return a1->timeout > a2->timeout; }
};

declare(PDict, AssignedAnalyzer);

class DPM {
public:
	DPM();
	~DPM();

	// Setup analyzer config.
	void PreScriptInit();	// To be called before scripts are parsed ...
	void PostScriptInit();	// ... and after.

	// Registers an analyzer with the DPM. This must be called only
	// before the script interpreter is initialized.
	void AddAnalyzer(AnalyzerTag tag,
		    const char* name,
		    analyzer_config_factory_callback factory,
		    analyzer_config_available_callback available,
		    analyzer_config_match_callback match,
		    bool partial);

	// Registers an analyzer for analysis on a port. The analyzer itself
	// must have been adeed with AddAnalyzer().
	void RegisterAnalyzerForPort(AnalyzerTag tag, TransportProto proto, uint32 port);

	// Given info about the first packet, build initial analyzer tree.
	//
	// It would be more flexible if we simply pass in the IP header
	// and then extract the information we need.  However, when this
	// method is called from the session management, protocol and ports
	// have already been extracted there and it would be a waste to do
	// it again.
	//
	// Returns 0 if we can't build a tree (e.g., because the necessary
	// analyzers have not been converted to the DPM framework yet...)

	bool BuildInitialAnalyzerTree(TransportProto proto, Connection* conn,
					const u_char* data);

	// Schedules a particular analyzer for an upcoming connection.
	// 0 acts as a wildcard for orig.  (Cookie is currently unused.
	// Eventually, we may pass it on to the analyzer).
	void ExpectConnection(const IPAddr& orig, const IPAddr& resp, uint16 resp_p,
				TransportProto proto, AnalyzerTag analyzer,
				double timeout, void* cookie);

	// Activates signature matching for protocol detection. (Called when an
	// DPM signatures is found.)
	void ActivateSigs()		{ sigs_activated = true; }
	bool SigsActivated() const	{ return sigs_activated; }

	void Done();

private:
	void AddConfigPreScriptInit(const AnalyzerConfig& tag);

	// Convert script-level config into internal data structures.
	void AddConfigPostScriptInit(const AnalyzerConfig& tag);

	// Return analyzer if any has been scheduled with ExpectConnection()
	// AnalyzerTag::::Error if none.
	AnalyzerTag GetExpected(int proto, const Connection* conn);

	// Mappings of destination port to analyzer.
	typedef list<AnalyzerTag> tag_list;
	typedef map<uint32, tag_list*> analyzer_map;
	analyzer_map tcp_ports;
	analyzer_map udp_ports;

	// True if signature-matching has been activated.
	bool sigs_activated;

	PDict(AssignedAnalyzer) expected_conns;

	typedef priority_queue<
			AssignedAnalyzer*,
			vector<AssignedAnalyzer*>,
			bool (*)(const AssignedAnalyzer*,
					const AssignedAnalyzer*)> conn_queue;
	conn_queue expected_conns_queue;
};

extern DPM* dpm;

#endif
