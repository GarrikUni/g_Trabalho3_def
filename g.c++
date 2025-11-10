#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <limits>
#include <tuple>

using namespace std;

// Estados para a Busca em Profundidade (DFS)
enum EstadoDFS { NAO_VISITADO, VISITADO, COMPLETO };


struct Atividade { // Cada vertice será uma atividade
    string id;              
    int duracao;            

    int ES = 0;             // Early Start 
    int EF = 0;             // Early Finish 
    
    int LS = numeric_limits<int>::max(); 
    int LF = numeric_limits<int>::max(); 

    int folga = 0;

    vector<string> precedentes; 
    vector<string> sucessores;  
    
    EstadoDFS estado = NAO_VISITADO;
};


void construirGrafo(map<string, Atividade>& atividades, const vector<tuple<string, int, string>>& dados) {
    // 1. Criar as atividades com duração e precedentes
    for (const auto& tupla : dados) {
        string id = get<0>(tupla);
        int duracao = get<1>(tupla);
        string precs_str = get<2>(tupla);

        atividades[id] = {id, duracao};
        stringstream ss(precs_str);
        string prec_id;

        // Processar os precedentes (ex: "A,B")
        while (getline(ss, prec_id, ',')) {
            if (!prec_id.empty() && prec_id != "-") {
                atividades[id].precedentes.push_back(prec_id);
            }
        }
    }

    // 2. Definir os sucessores (para facilitar a volta)
    for (auto const& pair : atividades) {
        const string& id = pair.first;
        for (const string& prec_id : pair.second.precedentes) {
            if (atividades.count(prec_id)) {
                atividades[prec_id].sucessores.push_back(id);
            }
        }
    }
}

// Função Recursiva DFS para Ordenação Topológica
bool dfsTopologicalSort(const string& id, map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    Atividade& ativ = atividades.at(id);
    ativ.estado = VISITADO;

    for (const string& sucessor_id : ativ.sucessores) {
        if (atividades.count(sucessor_id)) {
            Atividade& sucessor = atividades.at(sucessor_id);
            
            if (sucessor.estado == VISITADO) {
                // Ciclo detectado!
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
    ordem_topologica.push_back(id); // Adiciona ao início da lista
    return true; 
}

// Função Principal de Ordenação Topológica
bool realizarOrdenacaoTopologica(map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    ordem_topologica.clear();
    
    // Resetar o estado de visitação
    for (auto& pair : atividades) {
        pair.second.estado = NAO_VISITADO;
    }
    
    // Iniciar o DFS a partir de todas as atividades
    for (auto& pair : atividades) {
        if (pair.second.estado == NAO_VISITADO) {
            if (!dfsTopologicalSort(pair.first, atividades, ordem_topologica)) {
                return false; // Ciclo
            }
        }
    }
    
    // Invertemos para obter a ordem correta (Iniciais -> Finais)
    reverse(ordem_topologica.begin(), ordem_topologica.end());
    
    return true;
}


// 1. Ida: Calcula ES e EF
void forwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    for (const string& id : ordem_topologica) {
        Atividade& ativ = atividades.at(id);
        
        if (ativ.precedentes.empty()) {
            ativ.ES = 0;
        } else {
            // ES = max(EF dos precedentes)
            int max_ef = 0;
            for (const string& prec_id : ativ.precedentes) {
                if (atividades.count(prec_id)) {
                    max_ef = max(max_ef, atividades.at(prec_id).EF);
                }
            }
            ativ.ES = max_ef;
        }
        ativ.EF = ativ.ES + ativ.duracao;
    }
}

// 2. Volta: Calcula LS, LF e Folga
void backwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica, int duracao_projeto) {
    // Itera sobre a ordem topológica de trás para frente
    for (auto it = ordem_topologica.rbegin(); it != ordem_topologica.rend(); ++it) {
        const string& id = *it;
        Atividade& ativ = atividades.at(id);
        
        if (ativ.sucessores.empty()) {
            // Atividades finais
            ativ.LF = duracao_projeto;
        } else {
            // LF = min(LS dos sucessores)
            int min_ls = numeric_limits<int>::max();
            for (const string& suc_id : ativ.sucessores) {
                if (atividades.count(suc_id)) {
                    min_ls = min(min_ls, atividades.at(suc_id).LS);
                }
            }
            ativ.LF = min_ls;
        }
        
        // Calcula LS e Folga
        ativ.LS = ativ.LF - ativ.duracao;
        ativ.folga = ativ.LS - ativ.ES;
    }
}


vector<string> calcularPERT_CPM_Legacy(map<string, Atividade>& atividades, bool& sucesso) {
    vector<string> ordem_topologica;
    sucesso = true; 

    // Ordenação Topológica com DFS
    if (!realizarOrdenacaoTopologica(atividades, ordem_topologica)) {
        cout << "\nERRO: O cálculo PERT/CPM não pode ser completado devido a um ciclo no projeto." << endl;
        sucesso = false; 
        return ordem_topologica; 
    }
    cout << "Ordenação Topológica (DFS) completa. Ordem: ";
    for (const string& id : ordem_topologica) {
        cout << id << " ";
    }
    cout << endl;

    forwardPass(atividades, ordem_topologica);

    // Determinar a Duração do Projeto
    int duracao_projeto = 0;
    for (auto const& pair : atividades) {
        if (pair.second.sucessores.empty()) {
            duracao_projeto = max(duracao_projeto, pair.second.EF);
        }
    }
    cout << "--- Duração Mínima do Projeto: " << duracao_projeto << endl;


    backwardPass(atividades, ordem_topologica, duracao_projeto);

    return ordem_topologica; 
}


void exibirResultado(const map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    cout << "\n## Resultados do PERT/CPM\n";
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << "| Ativ | Dura | ES (Inicio Cedo) | EF (Fim Cedo) | LS (Inicio Tarde) | LF (Fim Tarde) | Folga | Critica |" << endl;
    cout << "--------------------------------------------------------------------------------" << endl;

    // Exibe a tabela na ordem topológica (para garantir a ordem do fluxo)
    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        string critico = (ativ.folga == 0) ? "SIM" : "NAO";
        
        printf("| %4s | %4d | %16d | %13d | %17d | %14d | %5d | %7s |\n",
               ativ.id.c_str(), ativ.duracao, ativ.ES, ativ.EF, ativ.LS, ativ.LF, ativ.folga, critico.c_str());
    }
    cout << "--------------------------------------------------------------------------------" << endl;

    // Exibir Caminho Crítico na Ordem Topológica garantida
    cout << "\n## Caminho Critico\n";
    cout << "Sequencia Critica: ";
    bool primeiro = true;

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
    // Tabela de Atividades, Duração e Precedentes:
    // Formato: {ID, Duração, Precedentes_IDs (separados por vírgula, use "-" se nenhum)}
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


    construirGrafo(atividades, dados_projeto);
    cout << "Grafo de Atividades Construido." << endl;


    bool calculo_ok;
    vector<string> ordem_topologica = calcularPERT_CPM_Legacy(atividades, calculo_ok);

    if (calculo_ok) {
        exibirResultado(atividades, ordem_topologica);
    }

    // Para compilar, use o comando:
    // g++ -o programa g.c++

    return 0;
}
