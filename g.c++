#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <limits>
#include <tuple>
#include <optional> // Necessário para usar std::optional

using namespace std;

// Estados para a Busca em Profundidade (DFS)
enum EstadoDFS { NAO_VISITADO, VISITADO, COMPLETO };

// Estrutura para cada Atividade (Vértice do Grafo)
struct Atividade {
    string id;              // Identificador da Atividade (ex: "A", "B", "C")
    int duracao;            // Duração da Atividade

    // Tempos de Início e Fim
    int ES = 0;             // Early Start (Início Mais Cedo)
    int EF = 0;             // Early Finish (Término Mais Cedo)
    int LS = numeric_limits<int>::max(); // Late Start (Início Mais Tarde)
    int LF = numeric_limits<int>::max(); // Late Finish (Término Mais Tarde)

    // Folga (Slack/Float)
    int folga = 0;

    // Relações de Precedência
    vector<string> precedentes; 
    vector<string> sucessores;  
    
    // Campo para o DFS
    EstadoDFS estado = NAO_VISITADO;
};

// --- Funções de Construção e Ordenação ---

void construirGrafo(map<string, Atividade>& atividades, const vector<tuple<string, int, string>>& dados) {
    for (const auto& tupla : dados) {
        string id = get<0>(tupla);
        int duracao = get<1>(tupla);
        string precs_str = get<2>(tupla);

        atividades[id] = {id, duracao};
        stringstream ss(precs_str);
        string prec_id;

        while (getline(ss, prec_id, ',')) {
            if (!prec_id.empty() && prec_id != "-") {
                atividades[id].precedentes.push_back(prec_id);
            }
        }
    }

    for (auto const& [id, ativ] : atividades) {
        for (const string& prec_id : ativ.precedentes) {
            if (atividades.count(prec_id)) {
                atividades[prec_id].sucessores.push_back(id);
            }
        }
    }
}

// Função Recursiva DFS para Ordenação Topológica
bool dfsTopologicalSort(const string& id, map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    Atividade& ativ = atividades[id];
    ativ.estado = VISITADO; 

    for (const string& sucessor_id : ativ.sucessores) {
        if (atividades.count(sucessor_id)) {
            Atividade& sucessor = atividades[sucessor_id];
            
            if (sucessor.estado == VISITADO) {
                cerr << "ERRO: Ciclo detectado no grafo: " << id << " -> " << sucessor_id << endl;
                return false; 
            }
            
            if (sucessor.estado == NAO_VISITADO) {
                if (!dfsTopologicalSort(sucessor_id, atividades, ordem_topologica)) {
                    return false; 
                }
            }
        }
    }

    ativ.estado = COMPLETO; 
    ordem_topologica.push_back(id); 
    return true; 
}

// Função Principal de Ordenação Topológica
bool realizarOrdenacaoTopologica(map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    ordem_topologica.clear();
    
    for (auto& par : atividades) {
        par.second.estado = NAO_VISITADO; // Resetar estado
    }
    
    for (auto& par : atividades) {
        if (par.second.estado == NAO_VISITADO) {
            if (!dfsTopologicalSort(par.first, atividades, ordem_topologica)) {
                return false; 
            }
        }
    }
    
    // Invertemos para obter a ordem correta (Iniciais -> Finais)
    reverse(ordem_topologica.begin(), ordem_topologica.end());
    
    return true;
}

// --- Funções de Cálculo ---

void forwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    for (const string& id : ordem_topologica) {
        Atividade& ativ = atividades[id];
        
        if (ativ.precedentes.empty()) {
            ativ.ES = 0;
        } else {
            int max_ef = 0;
            for (const string& prec_id : ativ.precedentes) {
                if (atividades.count(prec_id)) {
                    max_ef = max(max_ef, atividades[prec_id].EF);
                }
            }
            ativ.ES = max_ef;
        }
        ativ.EF = ativ.ES + ativ.duracao;
    }
}

void backwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica, int duracao_projeto) {
    for (auto it = ordem_topologica.rbegin(); it != ordem_topologica.rend(); ++it) {
        const string& id = *it;
        Atividade& ativ = atividades[id];
        
        if (ativ.sucessores.empty()) {
            ativ.LF = duracao_projeto;
        } else {
            int min_ls = numeric_limits<int>::max();
            for (const string& suc_id : ativ.sucessores) {
                if (atividades.count(suc_id)) {
                    min_ls = min(min_ls, atividades[suc_id].LS);
                }
            }
            ativ.LF = min_ls;
        }
        
        ativ.LS = ativ.LF - ativ.duracao;
        ativ.folga = ativ.LS - ativ.ES;
    }
}

// Retorna a ordem topológica ou nulo em caso de erro (ciclo)
optional<vector<string>> calcularPERT_CPM(map<string, Atividade>& atividades) {
    vector<string> ordem_topologica;

    // 1. Ordenação Topológica com DFS
    if (!realizarOrdenacaoTopologica(atividades, ordem_topologica)) {
        cout << "\n❌ O cálculo PERT/CPM não pode ser completado devido a um ciclo no projeto." << endl;
        return nullopt; // Falha (retorna opcional vazio)
    }
    cout << "✅ Ordenação Topológica (DFS) completa. Ordem: ";
    for (const string& id : ordem_topologica) {
        cout << id << " ";
    }
    cout << endl;

    // 2. Forward Pass
    forwardPass(atividades, ordem_topologica);

    // 3. Determinar a Duração do Projeto
    int duracao_projeto = 0;
    for (auto const& [id, ativ] : atividades) {
        if (ativ.sucessores.empty()) {
            duracao_projeto = max(duracao_projeto, ativ.EF);
        }
    }
    cout << "--- Duração Mínima do Projeto: " << duracao_projeto << endl;

    // 4. Backward Pass
    backwardPass(atividades, ordem_topologica, duracao_projeto);

    return ordem_topologica; // Sucesso (retorna a ordem)
}

// --- Função de Exibição (AGORA USANDO A ORDEM TOPOLÓGICA) ---

void exibirResultado(const map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    cout << "\n## 📊 Resultados do PERT/CPM\n";
    cout << "--------------------------------------------------------------------------------\n";
    cout << "| Ativ | Dura | ES (Início Cedo) | EF (Fim Cedo) | LS (Início Tarde) | LF (Fim Tarde) | Folga | Crítica |\n";
    cout << "--------------------------------------------------------------------------------\n";

    // Exibe a tabela na ordem topológica
    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        string critico = (ativ.folga == 0) ? "**SIM**" : "NÃO";
        
        printf("| %4s | %4d | %16d | %13d | %17d | %14d | %5d | %7s |\n",
               ativ.id.c_str(), ativ.duracao, ativ.ES, ativ.EF, ativ.LS, ativ.LF, ativ.folga, critico.c_str());
    }
    cout << "--------------------------------------------------------------------------------\n";

    // Exibir Caminho Crítico com Ordem Topológica garantida
    cout << "\n## 🚩 Caminho Crítico\n";
    cout << "Sequência Crítica: ";
    bool primeiro = true;

    // Itera SOMENTE sobre a lista de IDs ordenados e filtra os críticos
    for (const string& id : ordem_topologica) {
        if (atividades.at(id).folga == 0) {
            if (!primeiro) {
                cout << " -> ";
            }
            cout << id;
            primeiro = false;
        }
    }
    cout << endl;
}

int main() {
    // Exemplo de Tabela de Atividades, Duração e Precedentes:
    // Formato da tupla: {ID, Duração, Precedentes_IDs (separados por vírgula, use "-" se nenhum)}
    vector<tuple<string, int, string>> dados_projeto = {
        {"A", 2, "-"},
        {"B", 6, "K,L"},
        {"C", 10, "N"},
        {"D", 6, "C"},
        {"E", 4, "C"},
        {"F", 5, "E"},
        {"G", 7, "D"},
        {"H", 9, "E,G"},
        {"I", 7, "C"},
        {"J", 8, "F, I"},
        {"K", 4, "J"},
        {"L", 5, "J"},
        {"M", 2, "H"},
        {"N", 4, "A"}
    };

    map<string, Atividade> atividades;

    // 1. Construir o Grafo
    construirGrafo(atividades, dados_projeto);
    cout << "Grafo de Atividades Construído." << endl;

    // 2. Calcular PERT/CPM (incluindo DFS e retorna a ordem)
    if (auto ordem_optional = calcularPERT_CPM(atividades)) {
        // 3. Exibir Resultados, passando a ordem topológica
        exibirResultado(atividades, *ordem_optional);
    }

    return 0;
}
