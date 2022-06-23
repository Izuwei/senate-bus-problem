# The Senate Bus Problem

Implementation of the modified synchronization problem - The Senate Bus Problem. People (riders) come gradually to the stop and wait for the bus. When the bus arrives, all waiting people will board. Bus capacity is limited. If there are more people on the stop than the capacity of the bus, people who have not been in the bus are waiting for the next bus. If there is a bus at the stop and present people at the stop are boarding, other arrivals are always waiting for the next bus. After boarding, the bus leaves. If no one is waiting for the bus when he arrives at the stop, the bus leaves empty.

Example: The capacity of the bus is 10,  8 people are waiting on the stop. The bus arrives, people board, 3 more people come. Only 8 waiting people will board, the bus is leaving and 3 newcomers are waiting at the stop for next bus.

- Each person is represented by one process (rider).
- The bus is represented by the bus process. There is only one bus in the system.

## Run
```
make
./bus R C ART ABT
```  

where:  
- R is number of processes (riders)  
    R > 0

- C is capacity of the bus  
    C > 0  

- ART is the maximum value of the time (in milliseconds) for which a new rider process is generated  
    ART >= 0 && ART <= 1000  

- ABT is the maximum value of the time (in milliseconds) for which the bus process simulates driving  
    ABT >= 0 && ABT <= 1000  

- All parameters are integers
