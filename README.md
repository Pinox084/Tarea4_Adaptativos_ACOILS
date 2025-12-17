# Tarea4_Adaptativos_ACOILS

### Instrucciones de Uso

Comando de compilación: g++ hybrisACOILS.cpp -o hybrid

Ejemplo de ejecución: ./hybrid -i new_2000_dataset/erdos_n2000_p0c0.3_1.graph -t 10 --seed 42 --ants 20 --alpha 1.0 --beta 2.0 --rho 0.1 --ls_iters 2000

**Usar como nombre del compilable como hybrid para prevenir problemas con los scripts que utilizan ese nombre**

### Scripts

run.sh se usa para compilar con parametros ya predefinidos dentro del archivo hybridACOILS.cpp

test_final.sh se usa para poner a prueba dos sets de parametros, se uso para probar en densidades de 0.3 y 0.7
