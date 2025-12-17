#!/bin/bash


INSTANCES_DIR="./new_2000_dataset"  
EXEC="./hybrid"   #Ejecutable             
TIME_LIMIT=60

PARAMS_IRACE="--ants 21 --alpha 1.03 --beta 4.82 --rho 0.25 --ls_iters 4700"


PARAMS_MANUAL="--ants 20--alpha 1.0 --beta 2.0 --rho 0.1 --ls_iters 2000"


CSV_OUT="resultados_iracevsmanual_60_v2_2000.csv"



echo "densidad,algoritmo,sol_promedio,std_sol,tiempo_promedio,mejor_sol" > "$CSV_OUT"


declare -A sols_irace times_irace count_irace best_irace
declare -A sols_manual times_manual count_manual best_manual

echo "=== INICIANDO BENCHMARK EXPRESS (60s) ==="
echo "Directorios: $INSTANCES_DIR"
echo "Densidades objetivo: 0.3 y 0.7"
echo "-----------------------------------------------------------"

for file in "$INSTANCES_DIR"/*.graph; do
    [ -e "$file" ] || continue
    
    filename=$(basename "$file")
    
    #SOLO ARCHIVOS DE DENSIDAD 0.3 Y 0.7
    if [[ $filename =~ p0c(0\.[37])_ ]]; then
        densidad="${BASH_REMATCH[1]}"
    else
        continue
    fi

    echo -ne "Procesando $filename... "

   
    res_irace="Fail"
    res_manual="Fail"

    
    full_out_i=$($EXEC -i "$file" -t $TIME_LIMIT $PARAMS_IRACE 2>&1)
    
    
    out_i=$(echo "$full_out_i" | grep "FINAL_STATS:")

    if [[ -n "$out_i" ]]; then
        
        data=$(echo "$out_i" | cut -d' ' -f2) 
        s=$(echo "$data" | cut -d',' -f1)
        t=$(echo "$data" | cut -d',' -f3)

        sols_irace["$densidad"]+="$s "
        times_irace["$densidad"]+="$t "
        count_irace["$densidad"]=$((count_irace["$densidad"] + 1))
        
        res_irace=$s
        cur_best=${best_irace["$densidad"]:-0}
        if (( s > cur_best )); then best_irace["$densidad"]=$s; fi
    else
        
        echo "ERROR IRACE en $filename:" >> error.log
        echo "$full_out_i" >> error.log
    fi

    
    full_out_m=$($EXEC -i "$file" -t $TIME_LIMIT $PARAMS_MANUAL 2>&1)
    out_m=$(echo "$full_out_m" | grep "FINAL_STATS:")

    if [[ -n "$out_m" ]]; then
        data=$(echo "$out_m" | cut -d' ' -f2)
        s=$(echo "$data" | cut -d',' -f1)
        t=$(echo "$data" | cut -d',' -f3)

        sols_manual["$densidad"]+="$s "
        times_manual["$densidad"]+="$t "
        count_manual["$densidad"]=$((count_manual["$densidad"] + 1))

        res_manual=$s
        cur_best=${best_manual["$densidad"]:-0}
        if (( s > cur_best )); then best_manual["$densidad"]=$s; fi
    else
        echo "ERROR MANUAL en $filename:" >> error.log
        echo "$full_out_m" >> error.log
    fi

    echo "-> [Irace: $res_irace] vs [Manual: $res_manual]"
done

echo "-----------------------------------------------------------"
echo "Calculando estadísticas..."


calc_stats() {
    local vals_s=$1
    local vals_t=$2
    local n=$3
    
    awk -v s="$vals_s" -v t="$vals_t" -v n="$n" 'BEGIN {
        split(s, arr_s); split(t, arr_t);
        sum_s=0; sum_sq_s=0; sum_t=0;
        
        for(i=1; i<=n; i++) {
            v=arr_s[i]; 
            sum_s+=v; sum_sq_s+=v*v;
            sum_t+=arr_t[i];
        }
        
        avg_s = sum_s / n;
        std_s = sqrt((sum_sq_s - n*avg_s*avg_s) / n);
        avg_t = sum_t / n;
        
        printf "%.2f,%.4f,%.4f", avg_s, std_s, avg_t;
    }'
}


for dens in "0.3" "0.7"; do
    
    if [ "${count_irace[$dens]}" -gt 0 ]; then
        stats=$(calc_stats "${sols_irace[$dens]}" "${times_irace[$dens]}" "${count_irace[$dens]}")
        echo "p0c$dens,IRACE,$stats,${best_irace[$dens]}" >> "$CSV_OUT"
    fi

    
    if [ "${count_manual[$dens]}" -gt 0 ]; then
        stats=$(calc_stats "${sols_manual[$dens]}" "${times_manual[$dens]}" "${count_manual[$dens]}")
        echo "p0c$dens,MANUAL,$stats,${best_manual[$dens]}" >> "$CSV_OUT"
    fi
done

echo "¡Listo! Resultados guardados en $CSV_OUT"
column -s, -t "$CSV_OUT"