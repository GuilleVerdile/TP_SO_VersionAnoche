.PHONY: default

default: 
	cd FuncionesConexiones/Debug; make
	cd S-AFA/Debug; make
	cd CPU/Debug; make
	cd DAM/Debug; make
	cd FM9/Debug; make
	cd MDJ/Debug; make

clean:
	sudo rm -rf ~/so-commons-library
	cd FuncionesConexiones/Debug; make clean
	cd S-AFA/Debug; make clean
	cd CPU/Debug; make clean
	cd DAM/Debug; make clean
	cd FM9/Debug; make clean
	cd MDJ/Debug; make clean
