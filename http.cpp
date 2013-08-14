#include "http.h"
/**
using namespace std;

std::vector<string> http::urlUnVisited;
std::vector<string> http::urlVisited;

http::http(string url,int depth) {
	std::cout << "Criando Thread URL: " << url << std::endl;
	this->url = url;
	this->depth = depth;
	_lck = new Lock();
}
http::~http() {
	delete _lck;

}
void http::run() {
	try {
		cout << "Rodando" << endl;
		if (this->ParseUrl() == -1)
			throw;
		if (this->Server() == -1)
			throw;
		if (this->BuildQuery() == -1)
			throw;
		if (this->SendRequest() == -1)
			throw;
	} catch (...) {
		std::cout << ":3" << std::endl;
		this->~http();
	}
}

int http::Server( ) {
	int retorno = 0;
	try {
		this->socket_desc = socket(AF_INET , SOCK_STREAM , 0);
		if (this->socket_desc == -1)
			throw 0;

		this->host = gethostbyname(this->domain.c_str());
		if (this->host == NULL)
			throw 1;
		char * host_ip;
		host_ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
		std::cout << "host_ip: " << host_ip << std::endl;
		if (host_ip == NULL)
			throw 2;
		this->server.sin_family = AF_INET;
		this->server.sin_port = htons(PORT);
		this->server.sin_addr.s_addr = inet_addr(host_ip);

		for (int i = 0; i < 8; i++){
			this->server.sin_zero[i] = '\0';
		 }
	} catch (int e) {
		switch (e) {
		case 0:
			std::cout << "Não foi possivel criar o socket" << std::endl;
			break;
		case 1:
			std::cout << "Hostname inválido: " << this->domain << std::endl;
			break;
		case 2:
			std::cout << "inet_ntoa nulo" << std::endl;
			break;
		}

		retorno = -1;
	}

	return retorno;
}

std::vector<string> http::GetUrlList() {
	return http::urlUnVisited;
}


int http::ParseUrl() {
	int retorno = 0;
	try {
		std::cout << "URL: " << url << std::endl;
		vector<string> str_split;

		boost::split_regex(str_split, url,boost::regex("/"));
		this->path = "/";
		this->file = str_split[str_split.size()-1] == "" ? "index.html" : str_split[str_split.size()-1];

		std::size_t found = str_split[0].find("http");
		if (found != std::string::npos) {
			this->domain = str_split[2];
			for (int i = 3; i < (int)str_split.size();i++) {
				this->path += str_split[i];
				if (i < (int)str_split.size() - 1)
					this->path += "/";
			}
		} else {
			this->domain = str_split[0];
		}
	} catch (int e) {
		retorno = -1;
	}
	return retorno;
}
int http::BuildQuery() {
	int retorno = 0;
	try {
	    char request_temp[] = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";

	    this->http_query = (char *) malloc(strlen(this->domain.c_str()) + strlen(this->path.c_str()) + strlen(request_temp));
	    sprintf(this->http_query, request_temp, this->path.c_str(), this->domain.c_str());
	} catch (...) {
		retorno = -1;
	}
	return retorno;
}
int http::SendRequest() {
	int retorno = 0;
	try {
		if(send(this->socket_desc, this->http_query, strlen(this->http_query), 0) < 0)
			throw;
	} catch (...) {
		cout << "Falha no envio da requisição" << endl;
		retorno = -1;
	}
	return retorno;
}
int http::CreateDir() {
	int retorno = 0;
	try {

	} catch (int e) {
		retorno = -1;
	}
	return retorno;
}
int http::ReceiveData() {
	int retorno = 0;
	try {
		char server_reply[BUFSIZ];

		std::ofstream saida(filename.c_str(), ios::out);

		if (this->SendRequest() == -1)
			throw;

		int tam = 0;
		memset(server_reply,0,sizeof(server_reply));
		int header = 0;
		char * content = NULL;

		unsigned int ponteiro = 0;

		 Resposta do receive http 
		string resposta;
		resposta.clear();

		struct timeval tv;  timeval and timeout stuff added by davekw7x 
		int timeouts = 0;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		if (setsockopt(this->socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv)){
		  perror("setsockopt");
		}
		while((tam = recv(this->socket_desc, this->http_query ,BUFSIZ, 0)) > 0  && (++timeouts < 10000)) {
			resposta += this->http_query;
			//std::cout << "Tamanho: " << tam << std::endl;
			if (header == 0) {
				content = strstr(server_reply,"\r\n\r\n");
				if (content != NULL) {
					header = 1;
					content +=4;
					ponteiro = content - server_reply;
				}
			} else {
				ponteiro = 0;
				content = server_reply;
				//puts(content);
			}
			//std::cout << "Tamanho: " << tam << "content: " << sizeof(content)* strlen(content) << std::endl;

			//std::cout << "server_reply: " << strlen(server_reply) << std::endl;;
			//std::cout << "content: " << strlen(content) << std::endl;;
			//printf("Ponteiros: server_reply: %p content: %p \n",server_reply, content);
			//fwrite(content,tam,1,pFile);
			//std::cout << content << std::endl;
			saida.write(content,tam-ponteiro);

			memset(server_reply,0,tam);
		}

		size_t pos = resposta.find("\r\n\r\n");
		string headerStr( resposta.begin(), resposta.begin()+pos );
		std::size_t found = headerStr.find("text/html");
		if (found != std::string::npos) {

			//boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"|<img.+?src=[\"'](.+?)[\"'].+?>",
			boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"",
					   boost::regbase::normal | boost::regbase::icase);

			// lista de string acumulando os links parcialmente
			// depois colocar nos atributos da classe de urlVisited...
			list<string> l;

			//cout << "URL's encontradas " << ":" << endl;
			regex_split(back_inserter(l), resposta, e);

			boost::regex invalid("mailto:|javascript:");

			_lck->lock();
			while(l.size()){
				resposta = *(l.begin());
				l.pop_front();
				// Teste para verificar se no <a href> contem mailto: ou javascript:
				if(!(boost::regex_search(resposta, invalid))){
					if (!resposta.empty()){
						http::urlUnVisited.push_back(resposta);
					}
				}
			}
			_lck->unlock();
		}
		saida.close();

	} catch (int e) {
		retorno = -1;
	}
	return retorno;
}
**/
