/*
 * WebServer.cpp
 *
 *  Created on: May 19, 2017
 *      Author: kolban
 */
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <regex>
#include "sdkconfig.h"
#ifdef CONFIG_MONGOOSE_PRESENT
#include "WebServer.h"
#include <esp_log.h>
#include <mongoose.h>
#include <string>


static char tag[] = "WebServer";

/**
 * @brief Convert a Mongoose event type to a string.
 * @param [in] event The received event type.
 * @return The string representation of the event.
 */
static std::string mongoose_eventToString(int event) {
	switch (event) {
	case MG_EV_CONNECT:
		return "MG_EV_CONNECT";
	case MG_EV_ACCEPT:
		return "MG_EV_ACCEPT";
	case MG_EV_CLOSE:
		return "MG_EV_CLOSE";
	case MG_EV_SEND:
		return "MG_EV_SEND";
	case MG_EV_RECV:
		return "MG_EV_RECV";
	case MG_EV_POLL:
		return "MG_EV_POLL";
	case MG_EV_TIMER:
		return "MG_EV_TIMER";
	case MG_EV_HTTP_REQUEST:
		return "MG_EV_HTTP_REQUEST";
	case MG_EV_HTTP_REPLY:
		return "MG_EV_HTTP_REPLY";
	case MG_EV_HTTP_CHUNK:
		return "MG_EV_HTTP_CHUNK";
	case MG_EV_MQTT_CONNACK:
		return "MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNECT:
		return "MG_EV_MQTT_CONNECT";
	case MG_EV_MQTT_DISCONNECT:
		return "MG_EV_MQTT_DISCONNECT";
	case MG_EV_MQTT_PINGREQ:
		return "MG_EV_MQTT_PINGREQ";
	case MG_EV_MQTT_PINGRESP:
		return "MG_EV_MQTT_PINGRESP";
	case MG_EV_MQTT_PUBACK:
		return "MG_EV_MQTT_PUBACK";
	case MG_EV_MQTT_PUBCOMP:
		return "MG_EV_MQTT_PUBCOMP";
	case MG_EV_MQTT_PUBLISH:
		return "MG_EV_MQTT_PUBLISH";
	case MG_EV_MQTT_PUBREC:
		return "MG_EV_MQTT_PUBREC";
	case MG_EV_MQTT_PUBREL:
		return "MG_EV_MQTT_PUBREL";
	case MG_EV_MQTT_SUBACK:
		return "MG_EV_MQTT_SUBACK";
	case MG_EV_MQTT_SUBSCRIBE:
		return "MG_EV_MQTT_SUBSCRIBE";
	case MG_EV_MQTT_UNSUBACK:
		return "MG_EV_MQTT_UNSUBACK";
	case MG_EV_MQTT_UNSUBSCRIBE:
		return "MG_EV_MQTT_UNSUBSCRIBE";
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
		return "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		return "MG_EV_WEBSOCKET_HANDSHAKE_DONE";
	case MG_EV_WEBSOCKET_FRAME:
		return "MG_EV_WEBSOCKET_FRAME";
	case MG_EV_WEBSOCKET_CONTROL_FRAME:
		return "MG_EV_WEBSOCKET_CONTROL_FRAME";
	}
	std::ostringstream s;
	s << "Unknown event: " << event;
	return s.str();
} //eventToString

/**
 * @brief Convert a Mongoose string type to a string.
 * @param [in] mgStr The Mongoose string.
 * @return A std::string representation of the Mongoose string.
 */
static std::string mgStrToString(struct mg_str mgStr) {
	return std::string(mgStr.p, mgStr.len);
} // mgStrToStr

static void dumpHttpMessage(struct http_message *pHttpMessage) {
	ESP_LOGD(tag, "HTTP Message");
	ESP_LOGD(tag, "Message: %s", mgStrToString(pHttpMessage->message).c_str());
	ESP_LOGD(tag, "URI: %s", mgStrToString(pHttpMessage->uri).c_str());
}

/**
 * @brief Mongoose event handler.
 * The event handler is called when an event occurs associated with the WebServer
 * listening network connection.
 *
 * @param [in] mgConnection The network connection associated with the event.
 * @param [in] event The type of event.
 * @param [in] eventData Data associated with the event.
 * @return N/A.
 */
static void mongoose_event_handler_web_server(
	struct mg_connection *mgConnection, // The network connection associated with the event.
	int event, // The type of event.
	void *eventData // Data associated with the event.
) {
	if (event == MG_EV_POLL) {
		return;
	}
	ESP_LOGD(tag, "Event: %s", mongoose_eventToString(event).c_str());
	switch (event) {
	case MG_EV_HTTP_REQUEST: {
			struct http_message *message = (struct http_message *) eventData;
			dumpHttpMessage(message);

			WebServer *pWebServer = (WebServer *)mgConnection->user_data;
			pWebServer->processRequest(mgConnection, message);
			break;
		}
	} // End of switch
} // End of mongoose_event_handler


/**
 * @brief Constructor.
 */
WebServer::WebServer() {
	m_rootPath = "";
} // WebServer


WebServer::~WebServer() {
}


/**
 * @brief Get the current root path.
 * @return The current root path.
 */
std::string WebServer::getRootPath() {
	return m_rootPath;
} // getRootPath


/**
 * @brief Register a handler for a path.
 *
 * When a browser request arrives, the request will contain a method (GET, POST, etc) and a path
 * to be accessed.  Using this method we can register a regular expression and, if the incoming method
 * and path match the expression, the corresponding handler will be called.
 *
 * Example:
 * @code{.cpp}
 * static void handle_REST_WiFi(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
 *    ...
 * }
 *
 * webServer.addPathHandler("GET", "\\/ESP32/WiFi", handle_REST_WiFi);
 * @endcode
 *
 * @param [in] method The method being used for access ("GET", "POST" etc).
 * @param [in] pathExpr The path being accessed.
 * @param [in] handler The callback function to be invoked when a request arrives.
 */
void WebServer::addPathHandler(std::string method, std::string pathExpr, void (*handler)(WebServer::HTTPRequest *pHttpRequest, WebServer::HTTPResponse *pHttpResponse)) {
	m_pathHandlers.push_back(PathHandler(method, pathExpr, handler));
} // addPathHandler


/**
 * @brief Run the web server listening at the given port.
 *
 * This function does not return.
 *
 * @param [in] port The port number of which to listen.
 * @return N/A.
 */
void WebServer::start(uint16_t port) {
	ESP_LOGD(tag, "WebServer task starting");
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);

	std::stringstream stringStream;
	stringStream << ':' << port;
	struct mg_connection *mgConnection = mg_bind(&mgr, stringStream.str().c_str(), mongoose_event_handler_web_server);

	if (mgConnection == NULL) {
		ESP_LOGE(tag, "No connection from the mg_bind()");
		vTaskDelete(NULL);
		return;
	}

	mgConnection->user_data = this; // Save the WebServer instance reference in user_data.

	mg_set_protocol_http_websocket(mgConnection);

	ESP_LOGD(tag, "WebServer listening on port %d", port);
	while (1) {
		mg_mgr_poll(&mgr, 2000);
	}
} // run


/**
 * @brief Set the root path for URL file mapping.
 *
 * When a browser requests a file, it uses the address form:
 *
 * @code{.unparsed}
 * http://<host>:<port>/<path>
 * @endcode
 *
 * The path part can be considered the path to where the file should be retrieved on the
 * file system available to the web server.  Typically, we want a directory structure on the file
 * system to host the web served files and not expose the whole file system.  Using this method
 * we specify the root directory from which the files will be served.
 *
 * @param [in] path The root path on the file system.
 * @return N/A.
 */
void WebServer::setRootPath(std::string path) {
	m_rootPath = path;
} // setRootPath


/**
 * @brief Constructor.
 * @param [in] nc The network connection for the response.
 */
WebServer::HTTPResponse::HTTPResponse(struct mg_connection* nc) {
	m_nc = nc;
	m_status = 200;
	m_dataSent = false;
} // HTTPResponse


/**
 * @brief Add a header to the response.
 * @param [in] name The name of the header.
 * @param [in] value The value of the header.
 */
void WebServer::HTTPResponse::addHeader(std::string name, std::string value) {
	m_headers[name] = value;
} // addHeader


/**
 * @brief Send data to the HTTP caller.
 * Send the data to the HTTP caller.  No further data should be sent after this call.
 * @param [in] data The data to be sent to the HTTP caller.
 * @return N/A.
 */
void WebServer::HTTPResponse::sendData(std::string data) {
	sendData((uint8_t *)data.data(), data.length());
} // sendData


/**
 * @brief Send data to the HTTP caller.
 * Send the data to the HTTP caller.  No further data should be sent after this call.
 * @param [in] pData The data to be sent to the HTTP caller.
 * @param [in] length The length of the data to be sent.
 * @return N/A.
 */
void WebServer::HTTPResponse::sendData(uint8_t *pData, size_t length) {
	if (m_dataSent) {
		ESP_LOGE(tag, "HTTPResponse: Data already sent!  Attempt to send again/more.");
		return;
	}
	m_dataSent = true;

	std::map<std::string, std::string>::iterator iter;
	std::string headers;

	for (iter = m_headers.begin(); iter != m_headers.end(); iter++) {
		if (headers.length() == 0) {
			headers = iter->first + ": " + iter->second;
		} else {
			headers = "; " + iter->first + "=" + iter->second;
		}
	}
	mg_send_head(m_nc, m_status, length, headers.c_str());
	mg_send(m_nc, pData, length);
	m_nc->flags |= MG_F_SEND_AND_CLOSE;
} // sendData


/**
 * @brief Set the headers to be sent in the HTTP response.
 * @param [in] header The complete set of headers to send to the caller.
 * @return N/A.
 */
void WebServer::HTTPResponse::setHeaders(std::map<std::string, std::string>  headers) {
	m_headers = headers;
} // setHeaders


/**
 * @brief Get the current root path.
 * @return The current root path.
 */
std::string WebServer::HTTPResponse::getRootPath() {
	return m_rootPath;
} // getRootPath


/**
 * @brief Set the root path for URL file mapping.
 * @param [in] path The root path on the file system.
 * @return N/A.
 */
void WebServer::HTTPResponse::setRootPath(std::string path) {
	m_rootPath = path;
} // setRootPath


/**
 * @brief Set the status value in the HTTP response.
 *
 * The default if not set is 200.
 * @param [in] status The HTTP status code sent to the caller.
 * @return N/A.
 */
void WebServer::HTTPResponse::setStatus(int status) {
	m_status = status;
} // setStatus


/**
 * @brief Process an incoming HTTP request.
 *
 * We look at the path of the request and see if it has a matching path handler.  If it does,
 * we invoke the handler function.  If it does not, we try and find a file on the file system
 * that would resolve to the path.
 *
 * @param [in] mgConnection The network connection on which the request was received.
 * @param [in] message The message representing the request.
 */
void WebServer::processRequest(struct mg_connection *mgConnection, struct http_message* message) {
	std::string uri = mgStrToString(message->uri);
	ESP_LOGD(tag, "Matching: %s", uri.c_str());
	HTTPResponse httpResponse = HTTPResponse(mgConnection);
	httpResponse.setRootPath(getRootPath());


	/*
	 * Iterate through each of the path handlers looking for a match with the specified path.
	 */
	std::vector<PathHandler>::iterator it;
	for (it = m_pathHandlers.begin(); it < m_pathHandlers.end(); it++) {
		if ((*it).match(uri)) {
			HTTPRequest httpRequest(message);
			(*it).invoke(&httpRequest, &httpResponse);
			ESP_LOGD(tag, "Found a match!!");
			return;
		}
	} // End of examine path handlers.


	std::string filePath = httpResponse.getRootPath() + uri;
	ESP_LOGD(tag, "Opening file: %s", filePath.c_str());
	FILE *file = fopen(filePath.c_str(), "r");
	if (file != nullptr) {
		fseek(file, 0L, SEEK_END);
		size_t length = ftell(file);
		fseek(file, 0L, SEEK_SET);
		uint8_t *pData = (uint8_t *)malloc(length);
		fread(pData, length, 1, file);
		fclose(file);
		httpResponse.sendData(pData, length);
		free(pData);
	} else {
		// Handle unable to open file
		httpResponse.setStatus(404); // Not found
		httpResponse.sendData("");
	}
} // processRequest


/**
 * @brief Construct an instance of a PathHandler.
 *
 * @param [in] method The method to be matched.
 * @param [in] pathPattern The path pattern to be matched.
 * @param [in] webServerRequestHandler The request handler to be called.
 */
WebServer::PathHandler::PathHandler(std::string method, std::string pathPattern, void (*webServerRequestHandler)(WebServer::HTTPRequest *pHttpRequest, WebServer::HTTPResponse *pHttpResponse)) {
	m_method         = method;
	m_pattern        = std::regex(pathPattern);
	m_requestHandler = webServerRequestHandler;
} // PathHandler


/**
 * @brief Determine if the path matches.
 *
 * @param [in] path The path to be matched.
 * @return True if the path matches.
 */
bool WebServer::PathHandler::match(std::string path) {
	//ESP_LOGD(tag, "match: %s with %s", m_pattern.c_str(), path.c_str());
	return std::regex_search(path, m_pattern);
} // match


/**
 * @brief Invoke the handler.
 * @param [in] request An object representing the request.
 * @param [in] response An object representing the response.
 * @return N/A.
 */
void WebServer::PathHandler::invoke(WebServer::HTTPRequest* request, WebServer::HTTPResponse *response) {
	m_requestHandler(request, response);
} // invoke


/**
 * @brief Create an HTTPRequest instance.
 * When mongoose received an HTTP request, we want to encapsulate that to hide the
 * mongoose complexities.  We create an instance of this class to hide those.
 * @param [in] message The description of the underlying Mongoose message.
 */
WebServer::HTTPRequest::HTTPRequest(struct http_message* message) {
	m_message = message;
} // HTTPRequest


/**
 * @brief Get the body of the request.
 * When an HTTP request is either PUT or POST then it may contain a payload that is also
 * known as the body.  This method returns that payload (if it exists).
 * @return The body of the request.
 */
std::string WebServer::HTTPRequest::getBody() {
	return mgStrToString(m_message->body);
} // getBody


/**
 * @brief Get the method of the request.
 * An HTTP request contains a request method which is one of GET, PUT, POST, etc.
 * @return The method of the request.
 */
std::string WebServer::HTTPRequest::getMethod() {
	return mgStrToString(m_message->method);
} // getMethod


/**
 * @brief Get the path of the request.
 * The path of an HTTP request is the portion of the URL that follows the hostname/port pair
 * but does not include any query parameters.
 * @return The path of the request.
 */
std::string WebServer::HTTPRequest::getPath() {
	return mgStrToString(m_message->uri);
} // getPath

#define STATE_NAME 0
#define STATE_VALUE 1
/**
 * @brief Get the query part of the request.
 * The query is a set of name = value pairs.  The return is a map keyed by the name items.
 *
 * @return The query part of the request.
 */
std::map<std::string, std::string> WebServer::HTTPRequest::getQuery() {
	// Walk through all the characters in the query string maintaining a simple state machine
	// that lets us know what we are parsing.
	std::map<std::string, std::string> queryMap;
	std::string queryString = mgStrToString(m_message->query_string);
	int i=0;

	/*
	 * We maintain a simple state machine with states of:
	 * * STATE_NAME - We are parsing a name.
	 * * STATE_VALUE - We are parsing a value.
	 */
	int state = STATE_NAME;
	std::string name = "";
	std::string value;
	// Loop through each character in the query string.
	for (i=0; i<queryString.length(); i++) {
		char currentChar = queryString[i];
		if (state == STATE_NAME) {
			if (currentChar != '=') {
				name += currentChar;
			} else {
				state = STATE_VALUE;
				value = "";
			}
		} // End state = STATE_NAME
		else if (state == STATE_VALUE) {
			if (currentChar != '&') {
				value += currentChar;
			} else {
				//ESP_LOGD(tag, "name=%s, value=%s", name.c_str(), value.c_str());
				queryMap[name] = value;
				state = STATE_NAME;
				name = "";
			}
		} // End state = STATE_VALUE
	} // End for loop
	if (state == STATE_VALUE) {
		//ESP_LOGD(tag, "name=%s, value=%s", name.c_str(), value.c_str());
		queryMap[name] = value;
	}
	return queryMap;
} // getQuery


/**
 * @brief Return the constituent parts of the path.
 * If we imagine a path as composed of parts separated by slashes, then this function
 * returns a vector composed of the parts.  For example:
 *
 * ```
 * /x/y/z
 * ```
 * will break out to:
 *
 * ```
 * path[0] = ""
 * path[1] = "x"
 * path[2] = "y"
 * path[3] = "z"
 * ```
 *
 * @return A vector of the constituent parts of the path.
 */
std::vector<std::string> WebServer::HTTPRequest::pathSplit() {
	std::istringstream stream(getPath());
	std::vector<std::string> ret;
	std::string pathPart;
	while(std::getline(stream, pathPart, '/')) {
		ret.push_back(pathPart);
	}
	// Debug
	for (int i=0; i<ret.size(); i++) {
		ESP_LOGD(tag, "part[%d]: %s", i, ret[i].c_str());
	}
	return ret;
} // pathSplit

#endif // CONFIG_MONGOOSE_PRESENT
