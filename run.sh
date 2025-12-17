#!/bin/bash

SRC="hybridACOILS.cpp"
EXE="./hybrid"
DATA_DIR="new_3000_dataset"  
TIME_LIMIT=10
PARAMS="--ants 20 --alpha 1.0 --beta 2.0 --rho 0.1 --ls_iters 2000"


SAMPLE=$(find "$DATA_DIR" -name "erdos_n*.graph" | head -n 1)

if [ -z "$SAMPLE" ]; then
    echo "ERROR: No se encontraron archivos .graph en '$DATA_DIR'."
    exit 1
fi

FILENAME=$(basename "$SAMPLE")

if [[ $FILENAME =~ erdos_n([0-9]+)_ ]]; then
    N="${BASH_REMATCH[1]}"
    echo "-> Dataset detectado: N = $N nodos"
else
    echo "ERROR: No se pudo determinar el tamaño N del archivo $FILENAME"
    exit 1
fi

OUTPUT="stats_n${N}_final.txt"
rm -f "$OUTPUT"
echo "densidad sol_prom sol_std t_prom" >> "$OUTPUT"

echo "Compilando..."
g++ -O3 -std=c++17 "${SRC}" -o "${EXE#./}"
if [ $? -ne 0 ]; then
    echo "Error de compilación."
    exit 1
fi

for d in {1..9}; do
    densidad="0.${d}"
    RAW_FILE="runs_n${N}_p0c${densidad}.dat"
    rm -f "$RAW_FILE"
    
    echo "Procesando N=$N | Densidad p0c${densidad} ..."

    for i in {1..30}; do
       
        graph="${DATA_DIR}/erdos_n${N}_p0c${densidad}_${i}.graph"

        if [ ! -f "$graph" ]; then
            continue
        fi

        line=$(${EXE} -i "$graph" -t "$TIME_LIMIT" $PARAMS 2>&1 | grep "FINAL_STATS:")

        if [ -z "$line" ]; then
            echo "ERROR en $graph"
            continue
        fi

        data=$(echo "$line" | awk -F': ' '{print $2}')
        best=$(echo "$data" | cut -d',' -f1)
        tbest=$(echo "$data" | cut -d',' -f3)

        echo "${densidad} ${i} ${best} ${tbest}" >> "$RAW_FILE"
    done

    if [ ! -f "$RAW_FILE" ]; then
        continue
    fi

    sol_prom=$(awk '{sum += $3; n++} END {if (n>0) print sum/n;}' "$RAW_FILE")
    t_prom=$(awk '{sum += $4; n++} END {if (n>0) print sum/n;}' "$RAW_FILE")
    sol_std=$(awk '{x+=$3; x2+=$3*$3; n++} END {if(n>0){mean=x/n; var=x2/n-mean*mean; if(var<0) var=0; print sqrt(var)}}' "$RAW_FILE")

    echo "p0c${densidad} ${sol_prom} ${sol_std} ${t_prom}" >> "$OUTPUT"
    rm -f "$RAW_FILE"
done

echo "Terminado. Resultados en: $OUTPUT"
column -t "$OUTPUT"