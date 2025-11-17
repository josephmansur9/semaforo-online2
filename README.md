# semaforo-online2

[link do video](https://youtube.com/shorts/gn1lHiRfrWM)

[Código da montagem](/código.c++)

Descrição da solução: 

Utilizamos um sistema de semáforo on-line. Se trata de uma vida principal, cujo sinal estará aberto durante a maior parte do tempo. Além disso, temos uma via adicional de apenas um sentido. Quando o sensor de proximidade detecta um carro na via auxiliar, o semáforo da principal fica vermelho, e o semáforo da auxiliar se abre. Quando não há mais carros na via auxiliar, o seu semáforo fecha, e os semáforos principais abrem novamente.

Para a conexão on-line, as informações coletadas são enviadas para um broker, hospedado no serviço HiveMQ. Nele, é possível consultar informações relacionadas a detecção de carros e os status dos semáforos. Também é possível atualizar as variáveis do projeto pela plataforma, por meio do método publish. 

Por fim, um sistema de detecção dia/noite também foi implementado, por meio de um sensor de luminosidade. Durante o dia (luminosidade alta), os semáforos funcionam normalmente. Porém, no caso da noite (luminosidade baixa), os semáforos entram no estado de piscar as luzes amarelas.

Contribuições: 
- Teodoro focou no código do sensor de proximidade;
- Matheus focou no sensor de luz (modo noturno);
- Joseph focou na montagem física do protótipo;
- Rebeca focou no código da conexão MQTT;
- Lorenzo focou no código dos farois;
- Pedro focou na montagem física do protótipo;
- Marcus focou no HiveMQ.