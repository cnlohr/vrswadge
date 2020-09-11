all : main openvrless

main : cnovr/main
	cp $^ $@

openvrless : cnovr/openvrless
	cp $^ $@

clean :
	rm -rf *.o *~ main openvrless
