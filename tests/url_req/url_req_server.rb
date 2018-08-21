#!/usr/bin/env ruby
# Test server for url_req from nlutils.
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# Requires Ruby 1.8.7 or greater installed as 'ruby'

require 'webrick'

class TestServer < WEBrick::HTTPServlet::AbstractServlet
	def service(request, response)
		response.body = "#{request.request_method}:\n\t" +
			"#{request.to_s.lines.to_a.join("\t")}"
	end
end

server = WEBrick::HTTPServer.new({:Port => 38212})

server.mount_proc '/reverse' do |req, resp|
	resp.body = req.body.to_s.reverse
	resp.status = 404 unless req.path =~ %r{\A/reverse/?\z}

	puts "Reverse: received #{req.body.inspect}, responding with #{resp.body.inspect}"
end
server.mount_proc '/delayed' do |req, resp|
	sleep 2
	resp.body = 'Delayed'
	resp.status = 404 unless req.path =~ %r{\A/delayed/?\z}
end
server.mount '/', TestServer

['INT', 'TERM'].each do |s|
	trap s do
		server.shutdown
	end
end

server.start
