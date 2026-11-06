
# TODO(Any): replace that with the path to the directory where you installed
# simis and created the project.
SIMICS_PROJECT ?=

.PHONY: all sync compile rv32-atomic run-test help

ifneq ($(strip $(test)),)
all: run-test
else
all: compile
endif

sync:
	@test -n "$(SIMICS_PROJECT)" || { printf '%s\n' 'Indica la ruta del proyecto: make SIMICS_PROJECT=/ruta/al/proyecto'; exit 1; }
	@test -f "$(SIMICS_PROJECT)/GNUmakefile" || { printf '%s\n' 'No se encontró GNUmakefile en el proyecto de Simics indicado.'; exit 1; }
	@mkdir -p "$(SIMICS_PROJECT)/modules" "$(SIMICS_PROJECT)/targets"
	cp -R modules/. "$(SIMICS_PROJECT)/modules/"
	cp -R targets/. "$(SIMICS_PROJECT)/targets/"

compile: sync
	$(MAKE) -C "$(SIMICS_PROJECT)" rv32-atomic

rv32-atomic: compile

help:
	@printf '%s\n' \
		'Usage: make [target] [options]' \
		'' \
		'  make compile              Copy files and build rv32-atomic.' \
		'  make sync                 Copy modules/ and targets/ to the Simics project.' \
		'  make test=filename.txt    Build and open Simics with the specified test.' \
		'  make help                 Show this help.' \
		'' \
		'Tests are located in targets/rv32i/programs/testing/.' \
		'In Simics, type run to execute the program.' \
		'Simics project: $(SIMICS_PROJECT)' \
		'Override the path with SIMICS_PROJECT=/path/to/project.'

run-test: compile
	@test -n "$(test)" || { printf '%s\n' 'Indica el archivo: make test=nombre.txt'; exit 1; }
	@test -f "$(SIMICS_PROJECT)/targets/rv32i/programs/testing/$(test)" || { printf '%s\n' 'No se encontró el archivo de prueba en targets/rv32i/programs/testing/.'; exit 1; }
	cd "$(SIMICS_PROJECT)" && ./simics targets/rv32i/rv32-atomic.simics "program=targets/rv32i/programs/testing/$(test)"
