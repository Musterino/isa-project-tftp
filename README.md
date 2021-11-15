# TFTPv2 klient

TFTP klient zpracovava parametry zadane uzivatelem a nasledne zasila READ/WRITE pozadavek na TFTP server. Parametry se nesmi opakovat pri zadavani pozadavku. 

Jsou zde implementovane rozsireni timeout option, transfer size option a block size option. Pri odmitnuti timeout nebo block size option je brana v potaz hodnota prijata ze serveru, pokud nejaka je. Pokud ne, je ignorovano dane rozsireni, ktere chybi v option acknowledgement packetu.

Parametry:
	-R nebo -W             	-   read nebo write pozadavek, povinny
        -d <cesta_k_souboru> 	-   cesta k souboru k prenosu, povinny
        -t <sekundy>            -   timeout rozsireni, volitelny
        -s <pocet_bytu>    	-   maximalni velikost bloku v nasobcich oktetu, volitelny
        -c <mod>            	-   mod prenosu, volitelny
        -a <adresa,port>    	-   IP adresa a port
                                    TFTP serveru,         volitelny


### Neni implementovano:
	- prenosovy mod ascii
	- horni hranice MTU u blocksize option
	- multicast
