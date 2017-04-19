# invoke SourceDir generated makefile for empty.pem3
empty.pem3: .libraries,empty.pem3
.libraries,empty.pem3: package/cfg/empty_pem3.xdl
	$(MAKE) -f E:\David\Teleco\Curso_2016-2017\ELCO\SensorGloveProject\Guante/src/makefile.libs

clean::
	$(MAKE) -f E:\David\Teleco\Curso_2016-2017\ELCO\SensorGloveProject\Guante/src/makefile.libs clean

