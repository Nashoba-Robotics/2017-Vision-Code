all::
	make -C turret/ $(MAKECMDGOALS)
	make -C gear/ $(MAKECMDGOALS)

clean::
	make clean -C turret/
	make clean -C gear/

