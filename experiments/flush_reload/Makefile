all: victim


attack_baseline: victim.c
	gcc -DENABLE_ATTACK -pthread victim.c -o victim

iguard_with_attack: victim.c
	gcc -DENABLE_ATTACK -Diguard_ENABLE_PROTECTION -pthread victim.c -o victim

iguard_without_attack: victim.c
	gcc -Diguard_ENABLE_PROTECTION -pthread victim.c -o victim

victim: victim.c
	gcc -pthread victim.c -o victim

leak_test: leak_test.c
	gcc -g -Os leak_test.c -o leak_test

clean:
	rm -f victim leak_test
