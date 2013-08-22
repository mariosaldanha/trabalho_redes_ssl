/**
 *
 * FUNÇÕES PARCIAIS
 *
 * **/

#include "lib_webcrawler.h"

std::vector<string> lista_urls;
static std::vector<string> lista_urls_visitadas;
static string fullpath;
static string host_atual, path_atual;

// SSL
static std::vector<string> lista_urls_final;
static bool https;
static bool first;
static int PORT;
SSL *ssl;

// https ou http
boost::regex secure("https");
boost::regex notsecure("http");

// CRIA O SOCKET
int create_socket(){
	int socket_desc;

	if ((socket_desc = socket(AF_INET , SOCK_STREAM , 0))== -1){
		std::cerr << "Não criou o socket" << std::endl;
        return -1;
    }

    return socket_desc;
}

// FAZ O PAPEL DO DNS
char *get_ip (char *domain){
	struct hostent *host;
	char *host_ip;

	host = gethostbyname(domain);
	if(host == NULL){
		std::cerr << "Erro gethostbyname" << std::endl;
		return NULL;
	}

	host_ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);

	return host_ip;
}

// IP PRO SOCKET
void socket_address(struct sockaddr_in *server, char *host){
	char *host_ip;
	host_ip = get_ip(host);
	if (host_ip == NULL) {
		server->sin_addr.s_addr = -1;
		return;
	}
	server->sin_family = AF_INET;
    server->sin_port = htons(PORT);
    server->sin_addr.s_addr = inet_addr(host_ip);

    for (int i = 0; i < 8; i++){
		server->sin_zero[i] = '\0';
     }
}

// ESTABELECE A CONEXÃO
void socket_connect(int *socket_desc, struct sockaddr_in *server){
	if (connect(*socket_desc, ( struct sockaddr *) server, sizeof(*server)) > 0) {
        std::cerr << "Erro de conexao" << endl;
    	return;
    }
}

// MONTA A REQUISIÇÃO
char *build_request(char *host, char *path) {
    char *request;
    char request_temp[] = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";

    request = (char *) malloc(strlen(host) + strlen(path) + strlen(request_temp));
    sprintf(request, request_temp, path, host);

    return request;
}

// ENVIA A REQUISIÇÃO
void send_request(int *socket, char *get) {

	if(send(*socket, get, strlen(get), 0) < 0){
		std::cerr << "Falha no envio da requisição" << endl;
		return ;
	}
}

// INICIALIZA SSL 
SSL_CTX *create_ctx(){   
	SSL_METHOD *method;
    SSL_CTX *ctx;

    method = (SSL_METHOD*)SSLv23_client_method();
    if (!(ctx = SSL_CTX_new(method)))
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

// CONECTA SSL
void connect_ssl(SSL_CTX *ctx,int *socket_desc){
	ssl = SSL_new(ctx);      			// cria novo estado de conexão ssl
    SSL_set_fd(ssl, *socket_desc);    
	if(SSL_connect(ssl) <= 0){
		fprintf(stderr, "Error attempting to connect\n");
		ERR_print_errors_fp(stderr);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return;
	}
}

// ENVIA A REQUISIÇÃO SSL
void send_request_ssl(char *get) {
	if(SSL_write(ssl, get, strlen(get)) < 0){
		std::cerr << "Falha no envio da requisição" << endl;
		return ;
	}
}

// RETORNA O DONO
string owner_ssl(){
	
	X509 *certificado;
	string line;
	
	boost::regex e("/",
				   boost::regbase::normal | boost::regbase::icase);
	// Pega o certificado
	if ((certificado = SSL_get_peer_certificate(ssl)) ){
       // printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(certificado), 0, 0);
       // cout << "Subject: " << line << endl;
        
        list<string> subject;
		regex_split(back_inserter(subject), line, e);
		list<string>::const_iterator i;
		
		line = X509_NAME_oneline(X509_get_issuer_name(certificado), 0, 0);
        //cout << "Issuer: " << line << endl;
        
        list<string> issuer;
        regex_split(back_inserter(issuer), line, e);
        
        boost::regex e("O=",
				   boost::regbase::normal | boost::regbase::icase);
		boost::regex cn("CN=",
				   boost::regbase::normal | boost::regbase::icase);
        
        string subjectO;
        // Verifica nome do dono 
        // Procurando pela organização
        for(i = subject.begin(); i != subject.end(); ++i){
			if(boost::regex_search(*i, e)){
				subjectO = *i;
				break;
			}
		}
		
		string issuerO;
        // Verifica nome do emissor
        for(i = issuer.begin(); i != issuer.end(); ++i){
			if(boost::regex_search(*i, e)){
				issuerO = *i;
				break;
			}
		}
		
		// Se não tiver organização procura por CN
		if(subjectO.empty() || issuerO.empty()){
			//cout << "sem organização" << endl;
			// Verifica nome do dono
	        for(i = subject.begin(); i != subject.end(); ++i){
				if(boost::regex_search(*i, cn)){
					subjectO = *i;
					break;
				}
			}
			
			// Verifica nome do emissor
	        for(i = issuer.begin(); i != issuer.end(); ++i){
				if(boost::regex_search(*i, cn)){
					issuerO = *i;
					break;
				}
			}
		}
		
		//cout << "subjectCN: " << subjectCN << " issuerCN: " << issuerCN << endl;
		// se for proprio dono
		if(subjectO == issuerO){
			//cout << issuerCN << " *" << endl;
			return subjectO + "\t[AUTO-ASSINADO]"; 
		}
		else{
			//cout << subjectCN << endl;
			return subjectO; 
		}
        X509_free(certificado);
	}
	
	return "";
}

// RECEBE OS DADOS
std::vector<string> receive_data(int *socket, char *host, char *path){
	std::vector<string> retorno;
	char server_reply[BUFSIZ];
   	char *message;

	message = build_request(host, path);
	vector<string> str_split;

	boost::split_regex(str_split, fullpath,boost::regex("/"));
	string filename = str_split[str_split.size()-1] == "" ? "index.html" : str_split[str_split.size()-1];
	std::ofstream saida(fullpath.c_str(), ios::out);
	
	// verifica envio http ou https
	if(https){
		send_request_ssl(message);
	}
	else{
		send_request(socket, message);
	}

	int tam = 0;
	memset(server_reply,0,sizeof(server_reply));
	int header = 0;
	char * content = NULL;

	int totalTam = 0;
	int totalTam2 = 0;
	unsigned int ponteiro = 0;

	/** Resposta do receive http **/
	string resposta;
	resposta.clear();
	/**/
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	if (setsockopt(*socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv)){
	  perror("setsockopt");
	}
	
	if(https){
		while((tam = SSL_read(ssl, server_reply ,BUFSIZ)) > 0) {
			resposta += server_reply;
			totalTam = totalTam + tam;
			//std::cerr << "Tamanho: " << tam << std::endl;
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
			}
			saida.write(content,tam-ponteiro);
	
			memset(server_reply,0,tam);
			totalTam2 = totalTam2 + tam;
		}
	}
	else{
		//Receive a reply from the server
		while((tam = recv(*socket, server_reply ,BUFSIZ, 0)) > 0) {
			resposta += server_reply;
			totalTam = totalTam + tam;
			//std::cerr << "Tamanho: " << tam << std::endl;
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
			}
			saida.write(content,tam-ponteiro);
	
			memset(server_reply,0,tam);
			totalTam2 = totalTam2 + tam;
		}
	}

	/** Printando a resposta que acumulou do recv
	 *  para confirmar que pegou todo conteudo da resposta do http
	* */
	//std::cerr << resposta;
	size_t pos = resposta.find("\r\n\r\n");
	string headerStr( resposta.begin(), resposta.begin()+pos );
	std::size_t found = headerStr.find("text/html");
	if (found != std::string::npos) {

		boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"",
				   boost::regbase::normal | boost::regbase::icase);

		// lista de string acumulando os links parcialmente
		// depois colocar nos atributos da classe de urlVisited...
		list<string> l;

		//std::cerr << "URL's encontradas " << ":" << endl;
		regex_split(back_inserter(l), resposta, e);

		boost::regex invalid("mailto:|javascript:");

		string temp;
		vector<string> aux;
		string nPath = path;
		while(l.size()){
			aux.clear();
			temp.clear();
			resposta = *(l.begin());
			l.pop_front();
			//cout << "resposta " << resposta << endl;
			// Teste para verificar se no <a href> contem mailto: ou javascript:
			if(!(boost::regex_search(resposta, invalid))){
				if (!resposta.empty()){
					boost::regex absoluto("http|https");
					if(!(boost::regex_search(resposta, absoluto))){
						if (resposta.at(0) == '/') {
							resposta.replace(resposta.begin(), resposta.begin()+1,"");
						}
						if(https){
							temp += "https://";
						}
						else{
							temp += "http://";
						}
						//std::cerr <<"temp 1 " << temp << endl;
						temp += host;
						//std::cerr <<"temp 2 " << temp << endl;

						boost::regex arquivo(".*");
						if (boost::regex_search(path, arquivo)) {
							boost::split_regex(aux, nPath, boost::regex("/"));
							for(int i = 0; i < (int)aux.size()-1; i++){
								temp += aux[i];
								if(i < (int)aux.size()-1){
									temp += "/";
								}
							}
						}
						else{
							temp += path;
						}
						//std::cerr <<"temp 3 " << temp << endl;
						resposta = temp + resposta;
					}
					//std::cerr <<"resposta " << resposta << endl;
					retorno.push_back(resposta);
				}
			}
		}
	}
	saida.close();

	return retorno;
}

void create_dir(char *host, char *path){
	int status;
	string diretorio;
	string host_dir = host;
	string path_dir = path;
	struct stat fileStat;

	diretorio += "saida_webcrawler/";
	if (stat(diretorio.c_str(),&fileStat) < 0) {
		if((status = mkdir(diretorio.c_str(), 0777)) < 0){
			std::cerr << "Erro ao criar o diretorio " << diretorio << endl;
		}

	}
	diretorio += host_dir;
	diretorio += "/";

	if (stat(diretorio.c_str(),&fileStat) < 0) {
		if((status = mkdir(diretorio.c_str(), 0777)) < 0){
			std::cerr << "Erro ao criar o diretorio " << diretorio << endl;
		}
	}

	//cria os diretorios recursivamentes, a partir do host_dir
	vector<string> str_split;

	string extensao = "";
	string temp;
	boost::split_regex(str_split, path_dir,boost::regex("/"));
	temp.clear();
	temp += diretorio;

	for (int i = 0; i < (int)str_split.size(); i++) {
		//std::cerr << str_split[i] << endl;
		if (!str_split[i].empty()) {
			std::size_t found = str_split[i].find("?");
			if (found != std::string::npos || !extensao.empty()) {
				if (!extensao.empty())
					temp += "_";
				extensao = ".html";
				if(i < (int)str_split.size() -1){
					temp += str_split[i];
				}
			} else {

				if(i < (int)str_split.size() -1){
					temp += str_split[i] + "/";
				}
				if (stat(temp.c_str(),&fileStat) < 0) {
					if((status = mkdir(temp.c_str(), 0777)) < 0){
						//std::cerr << "Erro ao criar o diretorio " << temp << endl;
					}
					chmod(temp.c_str(), 0777);
				}
			}
		}
	}
	fullpath.clear();
	//fullpath = diretorio;
	fullpath += temp;
	fullpath += str_split[str_split.size()-1];
	fullpath += extensao;
}

void FazTudo(string url, int depth) {

	vector<string> str_split;
	boost::split_regex(str_split, url,boost::regex("/"));
	string domain;
	string path = "/";
	string file = str_split[str_split.size()-1];

	// Identifica se é link absoluto
	boost::regex absoluto("http|https");
	
	if((boost::regex_search(str_split[0], absoluto))){
		
		if(boost::regex_search(url, secure)){
			//cout << "SECURE " << url  << endl;
			https = true;
			PORT = 443;
		}
		else if(boost::regex_search(url, notsecure)){
			https = false;
			PORT = 80;
		}
		
		domain = str_split[2];
		for (int i = 3; i < (int)str_split.size();i++) {
			path += str_split[i];
			if (i < (int)str_split.size() - 1)
				path += "/";
		}
	} else {	// link relativo
		domain = host_atual;
		path = path_atual + str_split[0];
	}

	char * host_test = (char *)domain.c_str();
	char * path_test = (char *)path.c_str();
	std::string new_url;
	bool existe = false;

	int socket_desc;
	struct sockaddr_in server;
	create_dir(host_test, path_test);
	socket_desc = create_socket();
	//cout << domain << "   " << path << endl;
	socket_address(&server, host_test);

	if ((int)server.sin_addr.s_addr == -1)
		return;

	socket_connect(&socket_desc, &server);
	string dono;
	SSL_CTX *ctx;
	string aux;
	if(https){
		 // Seta a conexão segura
		ctx = create_ctx();
		
		// Carrega os ceritificados de confiança
		 if(! SSL_CTX_load_verify_locations(ctx, "ca-certificates.crt", NULL)){
			fprintf(stderr, "Error loading trust store\n");
			ERR_print_errors_fp(stderr);
			SSL_CTX_free(ctx);
			//exit(1);
			return;
		}
		
		// Criando a conexão ssl
		connect_ssl(ctx, &socket_desc);
		
		dono = owner_ssl();
		/**
		boost::regex autoassinado("AUTO-ASSINADO",
				   boost::regbase::normal | boost::regbase::icase);  
		**/
		// Verifica o certificado
	    if(SSL_get_verify_result(ssl) != X509_V_OK)
	    {
			if(first){
				aux = url + "\n\t\t<" + dono + ">";
				lista_urls_visitadas.push_back(url);
				lista_urls_final.push_back(aux);
				cout << aux << endl;
			}
			printf("\t\t\tCertificado não confiável\n");
	    }
	}
	else if(first){
		aux = url;
		lista_urls_visitadas.push_back(url);
		lista_urls_final.push_back(aux);
		cout << aux << endl;
	}
	first = false;
	std::vector<string> lista = receive_data(&socket_desc, host_test, path_test);
	for (int i=0; i < (int) lista.size();i++) {
		new_url = lista[i];
		//std::cerr << "Verificando: " << new_url;
		 existe = false;
		for (int i = 0; i < (int) lista_urls_visitadas.size();i++) {
			if (lista_urls_visitadas[i] == new_url) {
				//std::cerr << "URL jah existe na lista:" << new_url << std::endl;
				//std::cerr << "Visitada " << lista_urls_visitadas[i] << endl;
				existe = true;
			}
		}
		if (!existe && depth >= 0) {
			if(boost::regex_search(new_url, secure)){
				aux = new_url + "\n\t\t<" + dono + ">";
				//cout << "SECURE " << new_url  << endl;
			}
			else{
				aux = new_url;
			}
			cout << aux << endl;
			lista_urls_visitadas.push_back(new_url);
			lista_urls_final.push_back(aux);
			
			host_atual = domain;
			path_atual = path;
			if (depth >= 0) {
				FazTudo(new_url,depth-1);
			}
		}
	}
	if(https){
		// Termina conexão ssl
		if(ssl != NULL){
			SSL_free(ssl);        	// ssl
			ssl = NULL;
		}
		if(ctx != NULL){
			SSL_CTX_free(ctx);      // contexto
			ctx = NULL;
		}
		
	}
	if((shutdown(socket_desc, 2)) < 0){
		std::cerr << "Erro ao fechar o socket" << endl;
	}
}

int main(int argc , char *argv[]) {
	string url;
	int depth;
	
	if(argc != 3){
		std::cerr << "parametros invalidos" << endl;
		return 0;
	}
	
	// inicializa as bibliotecas ssl
	SSL_library_init();
	OpenSSL_add_all_algorithms();  
	SSL_load_error_strings();
	
	url += argv[1];

	depth = atoi (argv[2]);
	first = true;
   	FazTudo(url, depth);
   	
	//std::cout << "Lista URL visitadas:" << std::endl;
	//for (int i = 0; i < (int)lista_urls_final.size(); i++) {
	//	std::cout << lista_urls_final[i] << std::endl;
	//}

	return 0;
}
