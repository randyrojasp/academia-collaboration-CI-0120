# Pruebas del pipeline RV32

Los [programas del pipeline](../targets/rv32i/programs/testing/) son archivos
`.txt` como las demás pruebas e incluyen comentarios con los registros, el PC
y el motivo de detención esperados. La prueba
`pipeline_fetch_error.txt` supone la RAM predeterminada de 1 MiB y todos suponen
carga en la dirección 0.

Para abrir una prueba individual con el núcleo pipeline, desde la raíz del
repositorio:

```sh
make test=pipeline_branches.txt
```

Los archivos llamados `pipeline_*.txt` seleccionan automáticamente el núcleo
pipeline. En Simics, escribe `run` para ejecutarla y `pregs` para revisar los
registros. Los demás archivos siguen usando el núcleo atómico; para ejecutar
uno de ellos con el pipeline, usa por ejemplo:

```sh
make pipeline test=test_shifts.txt
```

Si el proyecto Simics está en otra ruta, se puede indicar con
`SIMICS_PROJECT=/ruta/al/proyecto` en cualquiera de los comandos anteriores.
