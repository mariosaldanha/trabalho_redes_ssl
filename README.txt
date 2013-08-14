MALE Webcrawler:

Grupo:
André Felipe da Silva - CComp
Mario Saldanha - CComp
Lucio Bastos - CComp
Eliezer Ribeiro - CComp

Repositorio GIT: https://github.com/andresilvaprogramador/trabalho_redes_-_webcrawler.git

Implementações:
	Foram utilizadas duas abordagens, uma recursiva, utilizando funcoes de criacao de conexao, envio de resposta e recebimento de dados, em funcoes separadas;
	chamando uma funcao principal - FazTudo() - recursivamente, utilizando uma lista estatica de links visitados para controle;
	A segunda abordagem não foi concluida, que seria a abordagem orientada a objetos utilizando threads, ambos os codigos estão nos arquivos fontes, porem
	apenas o metodo recursivo está funcional;
	
	o script executeme possui instrucoes de uso:
		./executeme.sh <url> <profundidade>
		
	alem de outros argumentos para facilitar a compilacao e instalacao de bibliotecas necessarias
      -h : chama ajuda
      -c : compila binario
      -i : instala bibliotecas necessarias para compilacao
      
Testes:

	Foram realizados testes com diversos sites, e alguns dos problemas encontrados foram:
	- A necessidade de tratar links relativos que não continham o dominio;
	- Criacao recursiva de pastas (http://dominio.com.br/exemplo/outro_exemplo/segundo_exemplo/;
	- Links com nome de arquivos implicitos (http://dominio.com.br/exemplo/;
