all: victim


attack_baseline: victim.c libsc.c
	gcc -DENABLE_ATTACK -pthread libsc.c victim.c -o victim

iguard_with_attack: victim.c libsc.c
	gcc -DENABLE_ATTACK -Diguard_ENABLE_PROTECTION -pthread libsc.c victim.c -o victim

iguard_without_attack: victim.c libsc.c
	gcc -Diguard_ENABLE_PROTECTION -pthread libsc.c victim.c -o victim

victim: victim.c libsc.c
	gcc -pthread libsc.c victim.c -o victim

clean:
	rm -f victim leak_test
