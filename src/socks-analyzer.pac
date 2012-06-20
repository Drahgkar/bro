
%header{
StringVal* array_to_string(vector<uint8> *a);
%}

%code{
StringVal* array_to_string(vector<uint8> *a)
	{
	int len = a->size();
	char tmp[len];
	char *s = tmp;
	for ( vector<uint8>::iterator i = a->begin(); i != a->end(); *s++ = *i++ );

	while ( len > 0 && tmp[len-1] == '\0' )
		--len;

	return new StringVal(len, tmp);
	}
%}

refine connection SOCKS_Conn += {

	function socks4_request(request: SOCKS4_Request): bool
		%{
		StringVal *dstname = 0;
		if ( ${request.v4a} )
			dstname = array_to_string(${request.name});
		else
			dstname = new StringVal("");
		
		BifEvent::generate_socks_request(bro_analyzer(),
		                                 bro_analyzer()->Conn(),
		                                 4,
		                                 ${request.command},
		                                 new AddrVal(htonl(${request.addr})),
		                                 dstname,
		                                 new PortVal(${request.port} | TCP_PORT_MASK),
		                                 array_to_string(${request.user}));

		static_cast<SOCKS_Analyzer*>(bro_analyzer())->EndpointDone(true);

		return true;
		%}

	function socks4_reply(reply: SOCKS4_Reply): bool
		%{
		BifEvent::generate_socks_reply(bro_analyzer(),
		                               bro_analyzer()->Conn(),
		                               4,
		                               ${reply.status},
		                               new AddrVal(htonl(${reply.addr})),
		                               new StringVal(""),
		                               new PortVal(${reply.port} | TCP_PORT_MASK));

		bro_analyzer()->ProtocolConfirmation();
		static_cast<SOCKS_Analyzer*>(bro_analyzer())->EndpointDone(false);
		return true;
		%}

	function socks5_request(request: SOCKS5_Request): bool
		%{
		AddrVal *ip_addr = 0;
		StringVal *domain_name = 0;
		
		// This is dumb and there must be a better way (checking for presence of a field)...
		switch ( ${request.remote_name.addr_type} )
			{
			case 1:
				ip_addr = new AddrVal(htonl(${request.remote_name.ipv4}));
				break;
			
			case 3:
				domain_name = new StringVal(${request.remote_name.domain_name.name}.length(), 
				                            (const char*) ${request.remote_name.domain_name.name}.data());
				break;
			
			case 4:
				ip_addr = new AddrVal(IPAddr(IPv6, (const uint32_t*) ${request.remote_name.ipv6}, IPAddr::Network));
				break;
			}
		
		if ( ! ip_addr )
			ip_addr = new AddrVal(uint32(0));
		if ( ! domain_name )
			domain_name = new StringVal("");
		
		BifEvent::generate_socks_request(bro_analyzer(),
		                                 bro_analyzer()->Conn(),
		                                 5,
		                                 ${request.command},
		                                 ip_addr,
		                                 domain_name,
		                                 new PortVal(${request.port} | TCP_PORT_MASK),
		                                 new StringVal(""));

		static_cast<SOCKS_Analyzer*>(bro_analyzer())->EndpointDone(true);

		return true;
		%}

	function socks5_reply(reply: SOCKS5_Reply): bool
		%{
		AddrVal *ip_addr = 0;
		StringVal *domain_name = 0;
		
		// This is dumb and there must be a better way (checking for presence of a field)...
		switch ( ${reply.bound.addr_type} )
			{
			case 1:
				ip_addr = new AddrVal(htonl(${reply.bound.ipv4}));
				break;
			
			case 3:
				domain_name = new StringVal(${reply.bound.domain_name.name}.length(), 
				                            (const char*) ${reply.bound.domain_name.name}.data());
				break;
			
			case 4:
				ip_addr = new AddrVal(IPAddr(IPv6, (const uint32_t*) ${reply.bound.ipv6}, IPAddr::Network));
				break;
			}
		
		if ( ! ip_addr )
			ip_addr = new AddrVal(uint32(0));
		if ( ! domain_name )
			domain_name = new StringVal("");
		
		BifEvent::generate_socks_reply(bro_analyzer(),
		                               bro_analyzer()->Conn(),
		                               5,
		                               ${reply.reply},
		                               ip_addr,
		                               domain_name,
		                               new PortVal(${reply.port} | TCP_PORT_MASK));

		bro_analyzer()->ProtocolConfirmation();
		static_cast<SOCKS_Analyzer*>(bro_analyzer())->EndpointDone(false);
		return true;
		%}
		
};

refine typeattr SOCKS4_Request += &let {
	proc: bool = $context.connection.socks4_request(this);
};

refine typeattr SOCKS4_Reply += &let {
	proc: bool = $context.connection.socks4_reply(this);
};

refine typeattr SOCKS5_Request += &let {
	proc: bool = $context.connection.socks5_request(this);
};

refine typeattr SOCKS5_Reply += &let {
	proc: bool = $context.connection.socks5_reply(this);
};
