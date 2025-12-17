#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <stdexcept>

// ==========================================
// 1. Estructuras de Datos y Carga de Grafo
// ==========================================

struct Graph {
    int n;
    std::vector<std::vector<int>> adj;

    
    bool are_adjacent(int u, int v) const {
        const auto& neighbors = adj[u];
        
        return std::binary_search(neighbors.begin(), neighbors.end(), v);
    }
};


bool detectOneIndexed(const std::string &filename, int n) {
    std::ifstream f(filename);
    if (!f) throw std::runtime_error("Error abriendo archivo");
    int dummy; f >> dummy; 
    int u, v;
    while (f >> u >> v) {
        if (u < 0 || v < 0) continue;
        if (u == 0 || v == 0) return false;
        if (u == n || v == n) return true;
        if (u > n || v > n) return true;
    }
    return true; 
}

Graph loadGraph(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("No se pudo abrir archivo: " + filename);

    Graph G;
    file >> G.n;
    if (G.n <= 0) throw std::runtime_error("Número de vértices inválido");

    G.adj.assign(G.n, {});
    bool oneIndexed = detectOneIndexed(filename, G.n);

    file.clear();
    file.seekg(0, std::ios::beg);
    int dummy; file >> dummy;

    int u, v;
    while (file >> u >> v) {
        if (u < 0 || v < 0) continue;
        if (oneIndexed) { u--; v--; }
        if (u >= 0 && u < G.n && v >= 0 && v < G.n && u != v) {
            G.adj[u].push_back(v);
            G.adj[v].push_back(u);
        }
    }

    
    for (auto& neighbors : G.adj) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }

    return G;
}

// Local Search
void local_search(const Graph& G, std::vector<int>& S, int max_iters) {
    int n = G.n;
    std::vector<char> inS(n, 0);
    for (int u : S) inS[u] = 1;

    //Comprobar conflictos de la solucion
    std::vector<int> conflicts(n, 0);
    for (int u = 0; u < n; ++u) {
        int c = 0;
        for (int v : G.adj[u]) if (inS[v]) ++c;
        conflicts[u] = c;
    }
    //añadir nodos sin conflictos
    auto add = [&](int u){
        inS[u] = 1; S.push_back(u);
        for (int v : G.adj[u]) conflicts[v]++;
    };
    //eliminar nodo de la solucion
    auto remove = [&](int u){
        inS[u] = 0;
        
        for (int v : G.adj[u]) conflicts[v]--;
    };

    bool improved = true;
    int it = 0;

    
    while (improved && it < max_iters) {
        improved = false; ++it;
        for (int u = 0; u < n; ++u) {
            if (!inS[u] && conflicts[u] == 0) {
                add(u);
                improved = true;
            }
        }
    }

   
    
    improved = true;
    while (improved && it < max_iters) {
        improved = false; ++it;
        
        int u_in = -1, v_in = -1, x_out = -1;

       
        for (int u = 0; u < n; ++u) {
            if (inS[u] || conflicts[u] != 1) continue;
            
            
            int xu = -1;
            for (int v : G.adj[u]) if (inS[v]) { xu = v; break; }
            if (xu < 0) continue;

            
            for (int v = u + 1; v < n; ++v) {
                if (inS[v] || conflicts[v] != 1) continue;
                if (G.are_adjacent(u, v)) continue; 

                int xv = -1;
                for (int w : G.adj[v]) if (inS[w]) { xv = w; break; }
                
                
                if (xu == xv) { 
                    u_in = u; v_in = v; x_out = xu;
                    goto FoundSwap;
                }
            }
        }

        FoundSwap:
        if (u_in != -1) {
            remove(x_out);
            add(u_in);
            add(v_in);
            improved = true;
        }
    }

    
    std::vector<int> finalS;
    finalS.reserve(S.size());
    for (int u = 0; u < n; ++u) {
        if (inS[u]) finalS.push_back(u);
    }
    S = finalS;
}

//Estructura de parámetros para ACO Híbrido

struct HybridParams {
    int numAnts = 10;       // hormigas 
    double alpha = 1.0;     //  feromona
    double beta  = 2.0;     // heurística
    double rho   = 0.1;     // Evaporación
    double tau0  = 1.0;     // Feromona inicial
    int ls_iters = 1000;    // Límite iteraciones Local Search
};

struct Solution {
    std::vector<int> set;
    int value = 0;
    double timeFound = 0.0;
};

Solution runHybridACO(const Graph& G, const HybridParams& p, int timeLimitSec, int seed) {
    std::mt19937 rng(seed);
    
    int n = G.n;
    std::vector<double> tau(n, p.tau0);
    std::vector<double> eta(n);

   
    for (int i = 0; i < n; ++i)
        eta[i] = 1.0 / (G.adj[i].size() + 1.0);

    Solution globalBest;
    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now - start).count() >= timeLimitSec) break;

        std::vector<std::vector<int>> cycleSolutions; 

        //FASE DE HORMIGAS
        for (int k = 0; k < p.numAnts; ++k) {
            std::vector<int> currentSol;
            std::vector<bool> forbidden(n, false);
            std::vector<int> candidates(n);
            std::iota(candidates.begin(), candidates.end(), 0);

            
            while (!candidates.empty()) {
                
                std::vector<double> probs;
                probs.reserve(candidates.size());
                double sumParams = 0.0;

                
                for (int cand : candidates) {
                    double val = std::pow(tau[cand], p.alpha) * std::pow(eta[cand], p.beta);
                    probs.push_back(val);
                    sumParams += val;
                }

                if (sumParams == 0) break; 

                std::uniform_real_distribution<double> dist(0.0, sumParams);
                double r = dist(rng);
                double acc = 0.0;
                int selected = -1;

                for (size_t i = 0; i < probs.size(); ++i) {
                    acc += probs[i];
                    if (r <= acc) {
                        selected = candidates[i];
                        break;
                    }
                }
                if (selected == -1) selected = candidates.back();

                
                currentSol.push_back(selected);
                
                //actualizar lista de candidatos
                forbidden[selected] = true;
                for (int neighbor : G.adj[selected]) forbidden[neighbor] = true;

                
                std::vector<int> nextCandidates;
                nextCandidates.reserve(candidates.size());
                for (int cand : candidates) {
                    if (!forbidden[cand]) nextCandidates.push_back(cand);
                }
                candidates = nextCandidates;
            }

            //FASE ILS
            local_search(G, currentSol, p.ls_iters);

            cycleSolutions.push_back(currentSol);

            // Verificar Mejor Global
            if ((int)currentSol.size() > globalBest.value) {
                globalBest.value = (int)currentSol.size();
                globalBest.set = currentSol;
                auto tNow = std::chrono::high_resolution_clock::now();
                globalBest.timeFound = std::chrono::duration<double>(tNow - start).count();
                
                std::cout << "[MEJORA] Size: " << globalBest.value 
                          << " | Time: " << std::fixed << std::setprecision(4) << globalBest.timeFound << "s" << std::endl;
            }
        }

        //evaporar y actualizar feromonas
        for (int i = 0; i < n; ++i) {
            tau[i] *= (1.0 - p.rho);
            if (tau[i] < 0.0001) tau[i] = 0.0001; 
        }

        for (const auto& sol : cycleSolutions) {
            double delta = 1.0 / (double)(G.n - sol.size() + 1.0); 
            for (int node : sol) {
                tau[node] += delta;
            }
        }
        
        //parte elitista
        if (globalBest.value > 0) {
            double deltaElite = 2.0 / (double)(G.n - globalBest.value + 1.0);
            for (int node : globalBest.set) {
                tau[node] += deltaElite;
            }
        }
    }

    return globalBest;
}

// ==========================================
// 4. Main
// ==========================================

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " [-i] <instancia> -t <segundos> [opciones...]\n";
        return 1;
    }

    std::string filename = "";
    double timeLimit = 10.0;
    int seed = 42;
    HybridParams params;
    // Valores por defecto
    params.numAnts = 20;
    params.alpha = 1.0;
    params.beta = 2.0;
    params.rho = 0.1;
    params.ls_iters = 2000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i") { if (i + 1 < argc) filename = argv[++i]; }
        else if (arg == "-t") { if (i + 1 < argc) timeLimit = std::stod(argv[++i]); }
        else if (arg == "--seed") { if (i + 1 < argc) seed = std::stoi(argv[++i]); }
        else if (arg == "--ants") { if (i + 1 < argc) params.numAnts = std::stoi(argv[++i]); }
        else if (arg == "--alpha") { if (i + 1 < argc) params.alpha = std::stod(argv[++i]); }
        else if (arg == "--beta") { if (i + 1 < argc) params.beta = std::stod(argv[++i]); }
        else if (arg == "--rho") { if (i + 1 < argc) params.rho = std::stod(argv[++i]); }
        else if (arg == "--ls_iters") { if (i + 1 < argc) params.ls_iters = std::stoi(argv[++i]); }
        
        else if (filename.empty() && arg[0] != '-') { filename = arg; }
    }

    if (filename.empty()) {
        std::cerr << "Error: No se especificó archivo.\n";
        return 1;
    }

    try {
        Graph G = loadGraph(filename);
        
        auto startTotal = std::chrono::high_resolution_clock::now();
        
        
        Solution sol = runHybridACO(G, params, timeLimit, seed);
        
        auto endTotal = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTotal = endTotal - startTotal;

        
        // Formato: FINAL_STATS: Solucion,TiempoTotal,TiempoMejor
        std::cout << "FINAL_STATS: " 
                  << sol.value << "," 
                  << std::fixed << std::setprecision(4) << elapsedTotal.count() << "," 
                  << sol.timeFound << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}