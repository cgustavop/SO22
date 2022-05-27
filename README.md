# SO22cgv
Trabalho prático no âmbito da Unidade Curricular de Sistemas Operativos

## Scripts de teste
Dentro da pasta scripts encontram-se vários tipos de scripts python.

 - `basic_server.py`: inicializa um servidor com as configurações de `config.txt`;
 - `chad_server.py`: inicializa um servidor com as configurações de `config_mega.txt`;
 - `poor_server.py`: inicializa um servidor com as configurações de `config_unavailable.txt`;
 - `single_req.py`: faz um pedido de um só processamento sobre o ficheiro `teste.txt` (implica um servidor pronto);
 - `mega_req.py`: faz um pedido de vários processamentos sobre o ficheiro `teste.txt` (implica um servidor pronto);
 - `testes.py`: faz uma série de pedidos de processamento a um servidor cronometrando tempos (implica um servidor pronto);

### Como correr
```bash
$python3 scripts/<nome_do_script>.py
```

### Ficheiro teste disponível
Encontra-se disponível um ficheiro `teste.txt`, contendo o script do filme Shrek, para testes de processamentos.