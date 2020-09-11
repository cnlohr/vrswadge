all : main openvrless fullres

main : cnovr/main
	cp $^ $@

fullres : cnovr/fullres
	cp $^ $@

openvrless : cnovr/openvrless
	cp $^ $@

clean :
	rm -rf *.o *~ main openvrless
