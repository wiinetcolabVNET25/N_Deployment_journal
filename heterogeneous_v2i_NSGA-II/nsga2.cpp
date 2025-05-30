#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>
#include <random>
#include <map>
#include <sstream>
#include <algorithm>
#include <climits>
#include <chrono>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define N 500 // tamanho da populaçao - deve ser par
#define G 4 // quantidade de geraçoes

#define LI 0.01 // limite inferior
#define LS 0.5  // limite superior

#define TC 0.9  // taxa de cruzamento
#define TM1 0.1 // taxa de mutaçao 1
#define TM2 0.1 // taxa de mutaçao 2
#define TM3 0.1 // taxa de mutaçao 3
#define AM 0.01 // agressividade da mutaçao

#define QBL 20 // quantidade de elementos do baseline
#define QG 0   // quantidade de elementos do grasp
 
#define TF "../datasets/6_to_8am.csv"    // arquivo de entrada
#define CF "../clustering_results/cluster_cells.csv"  // arquivo de clusters
#define BL "../baseline_solution/baseline.csv" // arquivo de clusters


// objetivos
std::vector<int> totalAlocacao(2*N, 0);
std::vector<std::vector<int>> totalCobertura;

// variaveis de decisao
std::vector<std::vector<bool>> alocacao;

// parametros
std::vector<std::vector<int>> movimentacao;
std::vector<std::vector<int>> tempoInicial;
std::vector<int> cluster;
std::vector<int> contato;

// variaveis auxiliares
std::map<int, std::pair<int, int>> indiceParaCelula;
std::vector<int> baseline;
std::vector<std::vector<int>> grasp(QG);

// aleatoriedade
std::random_device rd;                         
std::mt19937 gen(rd()); 

// limites
int numeroCelulas = 0;
int numeroVeiculos = 0;
int numeroClusters = 0;
int tempoInicio = INT_MAX;

void leParametros(std::vector<int> tau)
{
    std::map<std::pair<int, int>, int> celulaParaIndice;
    std::map<int, int> veiculoParaIndice;
    std::ifstream file(TF);
    std::string line;
    while (std::getline(file, line)) 
    {
        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> parsedRow;
        while (std::getline(lineStream, cell, ';')) 
        {
            parsedRow.push_back(cell);
        }
        int veiculo, celula;
        std::pair<int, int> posCelula;
        int veiculoId = std::stoi(parsedRow[0]);
        int tempo = int(std::stod(parsedRow[1]));
        if(tempo < tempoInicio)
        {
            tempoInicio = tempo;
        }
        posCelula = {std::stoi(parsedRow[2]), std::stoi(parsedRow[3])};
        auto it = celulaParaIndice.find(posCelula);
        if(it == celulaParaIndice.end())
        {
            celula = celulaParaIndice.size();
            celulaParaIndice[posCelula] = celula;
            indiceParaCelula[celula] = posCelula;
        }
        else
        {
            celula = it->second;
        }
        auto it2 = veiculoParaIndice.find(veiculoId);
        if(it2 == veiculoParaIndice.end())
        {
            veiculo = veiculoParaIndice.size();
            veiculoParaIndice[veiculoId] = veiculo;
        }
        else
        {
            veiculo = it2->second;
        }
        numeroVeiculos = veiculoParaIndice.size();
        movimentacao.resize(numeroVeiculos, std::vector<int>());
        tempoInicial.resize(numeroVeiculos, std::vector<int>());
        if(std::find(movimentacao[veiculo].begin(), movimentacao[veiculo].end(), celula) == movimentacao[veiculo].end() && veiculo >= 0)
        {
            movimentacao[veiculo].push_back(celula);
            tempoInicial[veiculo].push_back(tempo);
        }
    }
    file.close();
    numeroCelulas = indiceParaCelula.size();
    cluster.resize(numeroCelulas, 0);
    std::ifstream file2(CF);
    while (std::getline(file2, line)) 
    {
        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> parsedRow;
        while (std::getline(lineStream, cell, ',')) 
        {
            parsedRow.push_back(cell);
        }
        std::pair<int, int> posCelula = {std::stoi(parsedRow[0]), std::stoi(parsedRow[1])};
        auto it = celulaParaIndice.find(posCelula);
        if(it != celulaParaIndice.end())
        {
            int c = std::stoi(parsedRow[2]);
            if(c >= 0)
            {
                cluster[it->second] = c;
                if(c > numeroClusters)
                {
                    numeroClusters = c;
                }
            }
        }
    }
    file2.close();
    numeroClusters ++;
    std::ifstream file3(BL);
    while (std::getline(file3, line)) 
    {
        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> parsedRow;
        while (std::getline(lineStream, cell, ',')) 
        {
            parsedRow.push_back(cell);
        }
        std::pair<int, int> posCelula = {std::stoi(parsedRow[0]), std::stoi(parsedRow[1])};
        auto it = celulaParaIndice.find(posCelula);
        if(it != celulaParaIndice.end())
        {
            baseline.push_back(it->second);
        }
    }
    file3.close();
    for(int i=0; i<QG; i++)
    {
        std::string graspFile = "";
        for(int t: tau)
        {
            graspFile = graspFile + "_" + std::to_string(t);
        }
        std::ifstream file4("grasp"+std::to_string(i+1)+graspFile+".csv");
        while (std::getline(file4, line)) 
        {
            std::stringstream lineStream(line);
            std::string cell;
            std::vector<std::string> parsedRow;
            while (std::getline(lineStream, cell, ',')) 
            {
                parsedRow.push_back(cell);
            }
            std::pair<int, int> posCelula = {std::stoi(parsedRow[0]), std::stoi(parsedRow[1])};
            auto it = celulaParaIndice.find(posCelula);
            if(it != celulaParaIndice.end())
            {
                grasp[i].push_back(it->second);
            }
        }
        file4.close();
    }
}

int carregaBaseline()
{
    if(baseline.size() < LI*numeroCelulas || QBL > N)
    {
        return 0;
    }
    int tamanho = LI*numeroCelulas;
    for(int i=0; i<QBL; i++)
    {
        for(int j=0; j<tamanho; j++)
        {
            alocacao[i][baseline[j]] = true;
        }
        totalAlocacao[i] = tamanho;
        if(QBL > 1)
        {
            tamanho += double(LS-LI) * double(numeroCelulas) / double(QBL-1);
        }
    }
    return QBL;
}

int carregaGrasp(int inicio)
{
    int quantidade = 0;
    for(int i=0; i<QG; i++)
    {
        if(grasp[i].size() > 0)
        {
            quantidade ++;
            for(int j=0; j<grasp[i].size(); j++)
            {
                alocacao[inicio+i][grasp[i][j]] = true;
            }
            totalAlocacao[inicio+i] = grasp[i].size();
        }
        else
        {
            return quantidade;
        }
    }
    return quantidade;
}

void inicializa(std::vector<int> tau)
{
    alocacao.resize(2*N, std::vector<bool>(numeroCelulas, false));
    totalCobertura.resize(2*N, std::vector<int>(numeroClusters, 0));
    contato.resize(numeroClusters);
    for(int i=0; i<numeroClusters; i++)
    {
        contato[i] = pow(3, i);
    }
    if(tau.size() < numeroClusters)
    {
        int t = tau[0];
        tau.clear();
        for(int i=0; i<numeroClusters; i++)
        {
            tau.push_back(t);
        }
    }
}

void geraPopulacaoInicial()
{
    std::uniform_int_distribution<> dist(int(LI*numeroCelulas), int(LS*numeroCelulas-1));
    int individuosBaseline = carregaBaseline();
    int individuosGrasp = carregaGrasp(individuosBaseline);
    for(int i=individuosBaseline+individuosGrasp; i<N; i++)
    {
        std::vector<int> listaVazia(numeroCelulas);
        std::iota(listaVazia.begin(), listaVazia.end(), 0);
        int tamanho = dist(gen);
        for(int j=0; j<tamanho; j++)
        {
            std::uniform_int_distribution<> dist2(0, listaVazia.size()-1);
            int pos = dist2(gen);
            alocacao[i][listaVazia[pos]] = true;
            totalAlocacao[i] ++;
            listaVazia.erase(listaVazia.begin()+pos);
        }
    }
}

void avalia(int formulacao, std::vector<int> tau)
{
    if(formulacao == 1)
    {
        for(int i=0; i<N; i++)
        {
            for(int j=0; j<numeroVeiculos; j++)
            {
                std::vector<int> contador(numeroClusters, 0);
                std::vector<bool> primeiroAteTau(numeroClusters, false);
                for(int k=0; k<movimentacao[j].size(); k++)
                {
                    int celula = movimentacao[j][k];
                    if(alocacao[i][celula])
                    {
                        contador[cluster[celula]] ++;
                        if(tempoInicial[j][celula] <= tempoInicio+tau[cluster[celula]])
                        {
                            primeiroAteTau[cluster[celula]] = true;
                        }
                    }
                }
                for(int k=0; k<numeroClusters; k++)
                {
                    if(primeiroAteTau[k] && contador[k] >= contato[k])
                    {
                        totalCobertura[i][k] ++;
                    }
                }
            }
        }
    }
    else if(formulacao == 2)
    {
        for(int i=0; i<N; i++)
        {
            for(int j=0; j<numeroVeiculos; j++)
            {
                int contador = 0;
                std::vector<bool> passou(numeroClusters, false);
                std::vector<bool> primeiroAteTau(numeroClusters, false);
                for(int celula: movimentacao[j])
                {
                    if(alocacao[i][celula])
                    {
                        contador ++;
                        passou[cluster[celula]] = true;
                    }
                }
                for(int k=0; k<numeroClusters; k++)
                {
                    if(passou[k] && contador >= contato[k])
                    {
                        totalCobertura[i][k] ++;
                    }
                }
            }
        }
    }
}

void gravaBenchmarks(int numeroExperimento)
{
    std::string dirName = "experimento " + std::to_string(numeroExperimento) + "/";
    DIR* dir = opendir(dirName.c_str());
    if (dir) {
        closedir(dir);
    } else {
        if (mkdir(dirName.c_str(), 0777) != 0) {
            std::cerr << "Error creating directory" << std::endl;
        }
    }
    int inicioGrasp = 0;
    if(baseline.size() > LI*numeroCelulas && QBL < N)
    {
        for(int i=0; i<QBL; i++)
        {
            std::ofstream myFile(dirName + "baseline-"+std::to_string(i+1)+".txt");
            myFile << "Total de RSUs:" << std::endl;
            myFile << totalAlocacao[i+N] << std::endl;
            for(int j=0; j<numeroClusters; j++)
            {
                myFile << "Cobertura no cluster "+std::to_string(j)+":" << std::endl;
                myFile << totalCobertura[i+N][j] << std::endl;
            } 
            myFile << "Celulas com RSU:" << std::endl;
            for(int j=0; j<numeroCelulas; j++)
            {
                if(alocacao[i+N][j])
                {
                    auto it = indiceParaCelula.find(j);
                    if (it != indiceParaCelula.end())
                    {
                        myFile << std::to_string(it->second.first)+","+std::to_string(it->second.second) << std::endl;
                    }
                }
            }
            myFile.close();
        }
        inicioGrasp = QBL;
    }
    for(int i=0; i<QG; i++)
    {
        int idx = i+inicioGrasp; 
        std::ofstream myFile(dirName + "grasp-"+std::to_string(i+1)+".txt");
        myFile << "Total de RSUs:" << std::endl;
        myFile << totalAlocacao[idx+N] << std::endl;
        for(int j=0; j<numeroClusters; j++)
        {
            myFile << "Cobertura no cluster "+std::to_string(j)+":" << std::endl;
            myFile << totalCobertura[idx+N][j] << std::endl;
        } 
        myFile << "Celulas com RSU:" << std::endl;
        for(int j=0; j<numeroCelulas; j++)
        {
            if(alocacao[idx+N][j])
            {
                auto it = indiceParaCelula.find(j);
                if (it != indiceParaCelula.end())
                {
                    myFile << std::to_string(it->second.first)+","+std::to_string(it->second.second) << std::endl;
                }
            }
        }
        myFile.close();
    }
}

void recombinacao(int filho, int tamanho, int pai1, int pai2, double proporcaoPai1)
{
    std::uniform_real_distribution<> dist(0.0, 1.0);
    std::vector<int> listaPai1, listaPai2, listaVazia;
    int tamanhoAtual = 0;
    std::fill(totalCobertura[filho].begin(), totalCobertura[filho].end(), 0);
    for(int i=0; i<numeroCelulas; i++)
    {
        alocacao[filho][i] = false;
        totalAlocacao[filho] = 0;
        if(alocacao[pai1][i])
        {
            listaPai1.push_back(i);
        }
        if(alocacao[pai2][i])
        {
            listaPai2.push_back(i);
        }
    }
    while(totalAlocacao[filho] < tamanho)
    {
        if(!listaPai1.empty() && (dist(gen) < proporcaoPai1 || listaPai2.empty()))
        {
            std::uniform_int_distribution<> dist2(0, listaPai1.size()-1);
            int pos = dist2(gen);
            if(!alocacao[filho][listaPai1[pos]])
            {
                alocacao[filho][listaPai1[pos]] = true;
                totalAlocacao[filho] ++;
            }
            listaPai1.erase(listaPai1.begin()+pos);
        }
        else if(!listaPai2.empty())
        {
            std::uniform_int_distribution<> dist2(0, listaPai2.size()-1);
            int pos = dist2(gen);
            if(!alocacao[filho][listaPai2[pos]])
            {
                alocacao[filho][listaPai2[pos]] = true;
                totalAlocacao[filho] ++;
            }
            listaPai2.erase(listaPai2.begin()+pos);
        }
        else
        {
            std::vector<int> listaVazia;
            for(int i=0; i<numeroCelulas; i++)
            {
                if(!alocacao[filho][i])
                {
                    listaVazia.push_back(i);
                }
            }
            while(totalAlocacao[filho] < tamanho)
            {
                std::uniform_int_distribution<> dist2(0, listaVazia.size()-1);
                int pos = dist2(gen);
                alocacao[filho][listaVazia[pos]] = true;
                totalAlocacao[filho] ++;
                listaVazia.erase(listaVazia.begin()+pos);
            }
        }
    }
}

void cruzamento()
{
    std::uniform_int_distribution<> dist(N, 2*N-1);
    std::uniform_real_distribution<> dist2(0.0, 1.0);
    for(int i=0; i<N; i+=2)
    {
        int pai1 = dist(gen);
        int pai2 = dist(gen);
        while(pai1 == pai2)
        {
            pai2 = dist(gen);
        }
        if(dist2(gen) > TC)
        {
            std::copy(alocacao[pai1].begin(), alocacao[pai1].end(), alocacao[i].begin());
            std::fill(totalCobertura[i].begin(), totalCobertura[i].end(), 0);
            totalAlocacao[i] = totalAlocacao[pai1];
            std::copy(alocacao[pai2].begin(), alocacao[pai2].end(), alocacao[i+1].begin());
            std::fill(totalCobertura[i+1].begin(), totalCobertura[i+1].end(), 0);
            totalAlocacao[i+1] = totalAlocacao[pai2];
            continue;
        }
        double proporcaoPai1 = dist2(gen);
        int media = (totalAlocacao[pai1]+totalAlocacao[pai2])/2.0;
        int desvio = std::abs(totalAlocacao[pai1]-totalAlocacao[pai2])/2.0;
        std::normal_distribution<> dist3(media, desvio);
        int tamanhoFilho = dist3(gen);
        int tentativas = 0;
        while(tamanhoFilho < LI*numeroCelulas || tamanhoFilho > LS*numeroCelulas) 
        {
            if(tentativas ++ == 10)
            {
                tamanhoFilho = media;
                break;
            }
            tamanhoFilho = dist3(gen);
        }
        recombinacao(i, tamanhoFilho, pai1, pai2, proporcaoPai1);
        recombinacao(i+1, tamanhoFilho, pai1, pai2, 1.0-proporcaoPai1);
    }
}

void mutacao1(int filho)
{
    std::vector<int> listaVazia;
    for(int i=0; i<numeroCelulas; i++)
    {
        if(!alocacao[filho][i])
        {
            listaVazia.push_back(i);
        }
    }
    int agressividade = std::max(1, int(totalAlocacao[filho]*AM));
    if(listaVazia.size() >= agressividade && totalAlocacao[filho]+agressividade <= LS*numeroCelulas)
    {
        for(int i=0; i<agressividade; i++)
        {
            std::uniform_int_distribution<> dist(0, listaVazia.size()-1);
            int pos = dist(gen);
            alocacao[filho][listaVazia[pos]] = true;
            totalAlocacao[filho] ++;
            listaVazia.erase(listaVazia.begin()+pos);
        }
    }
}

void mutacao2(int filho)
{
    std::vector<int> listaVazia;
    for(int i=0; i<numeroCelulas; i++)
    {
        if(alocacao[filho][i])
        {
            listaVazia.push_back(i);
        }
    }
    int agressividade = std::max(1, int(totalAlocacao[filho]*AM));
    if(listaVazia.size() >= agressividade && totalAlocacao[filho]-agressividade >= LI*numeroCelulas)
    {
        for(int i=0; i<agressividade; i++)
        {
            std::uniform_int_distribution<> dist(0, listaVazia.size()-1);
            int pos = dist(gen);
            alocacao[filho][listaVazia[pos]] = false;
            totalAlocacao[filho] --;
            listaVazia.erase(listaVazia.begin()+pos);
        }
    }
}

void mutacao3(int filho)
{
    std::vector<int> listaVazia;
    std::vector<int> listaCheia;
    for(int i=0; i<numeroCelulas; i++)
    {
        if(alocacao[filho][i])
        {
            listaVazia.push_back(i);
        }
        else
        {
            listaCheia.push_back(i);
        }
    }
    int agressividade = std::max(1, int(totalAlocacao[filho]*AM));
    if(listaVazia.size() >= agressividade && listaCheia.size() >= agressividade)
    {
        for(int i=0; i<agressividade; i++)
        {
            std::uniform_int_distribution<> dist(0, listaVazia.size()-1);
            std::uniform_int_distribution<> dist2(0, listaCheia.size()-1);
            int pos = dist(gen);
            int pos2 = dist2(gen);
            alocacao[filho][listaVazia[pos]] = true;
            listaVazia.erase(listaVazia.begin()+pos);
            alocacao[filho][listaCheia[pos2]] = false;
            listaCheia.erase(listaCheia.begin()+pos2);
        }
    }
}

void mutacao()
{
    std::uniform_real_distribution<> dist(0.0, 1.0);
    for(int i=0; i<N; i++)
    {
        if(dist(gen) < TM1)
        {
            mutacao1(i); // adiciona RSUs
        }
        if(dist(gen) < TM2)
        {
            mutacao2(i); // remove RSUs
        }
        if(dist(gen) < TM3)
        {
            mutacao3(i); // realoca RSUs
        }
    }
}

bool domina(int individuo1, int individuo2)
{
    bool domina = false;
    if(totalAlocacao[individuo1] < totalAlocacao[individuo2])
    {
        domina = true;
    }
    else if(totalAlocacao[individuo1] > totalAlocacao[individuo2])
    {
        return false;
    }
    for(int i=0; i<numeroClusters; i++)
    {
        if(totalCobertura[individuo1][i] > totalCobertura[individuo2][i])
        {
            domina = true;
        }
        else if(totalCobertura[individuo1][i] < totalCobertura[individuo2][i])
        {
            return false;
        }
    }
    return domina;
}

void ordenacaoNaoDominada(std::vector<int> &rank)
{
    std::vector<std::vector<int>> dominados(2*N);
    std::vector<int> numeroDomina(2*N, 0);
    std::vector<int> fronteira;
    for(int i=0; i<2*N; i++)
    {
        for(int j=0; j<2*N; j++)
        {
            if(domina(i, j))
            {
                dominados[i].push_back(j);
            }
            else if(domina(j, i))
            {
                numeroDomina[i] ++;
            }
        }
        if(numeroDomina[i] == 0)
        {
            rank[i] = 0;
            fronteira.push_back(i);
        }
    }
    int r = 0;
    while(fronteira.size() > 0)
    {
        std::vector<int> temporario;
        for(int individuo: fronteira)
        {
            for(int elemento: dominados[individuo])
            {
                numeroDomina[elemento] --;
                if(numeroDomina[elemento] == 0)
                {
                    rank[elemento] = r+1;
                    temporario.push_back(elemento);
                }
            }
        }
        r ++;
        fronteira.resize(temporario.size());
        std::copy(temporario.begin(), temporario.end(), fronteira.begin());
    }
}

void distanciaDeMultidao(std::vector<std::pair<double, int>> &distancia)
{
    int tamanho = distancia.size();
    std::vector<std::pair<int, int>> ordenaAlocacao(tamanho);
    int max = totalAlocacao[distancia[0].second];
    int min = totalAlocacao[distancia[0].second];
    for(int i=0; i<tamanho; i++)
    {
        ordenaAlocacao[i] = {totalAlocacao[distancia[i].second], i};
        if(totalAlocacao[distancia[i].second] > max)
        {
            max = totalAlocacao[distancia[i].second];
        }
        else if(totalAlocacao[distancia[i].second] < min)
        {
            min = totalAlocacao[distancia[i].second];
        }
    }
    std::sort(ordenaAlocacao.begin(), ordenaAlocacao.end());
    distancia[ordenaAlocacao[0].second].first = numeroClusters + 1.0;
    distancia[ordenaAlocacao[tamanho-1].second].first = numeroClusters + 1.0;
    for(int i=1; i<tamanho-1; i++)
    {
        int vizinhoMaior = distancia[ordenaAlocacao[i+1].second].second;
        int vizinhoMenor = distancia[ordenaAlocacao[i-1].second].second;
        distancia[ordenaAlocacao[i].second].first += double(totalAlocacao[vizinhoMaior]-totalAlocacao[vizinhoMenor])/double(max-min);
    }
    for(int j=0; j<numeroClusters; j++)
    {
        std::vector<std::pair<int, int>> ordenaCobertura(tamanho);
        int max = totalCobertura[distancia[0].second][j];
        int min = totalCobertura[distancia[0].second][j];
        for(int i=0; i<tamanho; i++)
        {
            ordenaCobertura[i] = {totalCobertura[distancia[i].second][j], i};
            if(totalCobertura[distancia[i].second][j] > max)
            {
                max = totalCobertura[distancia[i].second][j];
            }
            else if(totalCobertura[distancia[i].second][j] < min)
            {
                min = totalCobertura[distancia[i].second][j];
            }
        }
        std::sort(ordenaCobertura.begin(), ordenaCobertura.end());
        distancia[ordenaCobertura[0].second].first = numeroClusters + 1.0;
        distancia[ordenaCobertura[tamanho-1].second].first = numeroClusters + 1.0;
        for(int i=1; i<tamanho-1; i++)
        {
            int vizinhoMaior = distancia[ordenaCobertura[i+1].second].second;
            int vizinhoMenor = distancia[ordenaCobertura[i-1].second].second;
            distancia[ordenaCobertura[i].second].first += double(totalCobertura[vizinhoMaior][j]-totalCobertura[vizinhoMenor][j])/double(max-min);
        }
    }
}

void selecao()
{
    std::vector<int> rank(2*N);
    ordenacaoNaoDominada(rank);
    std::vector<std::vector<bool>> alocacaoTemp(N, std::vector<bool>(numeroCelulas));
    std::vector<std::vector<int>> totalCoberturaTemp(N, std::vector<int>(numeroClusters));
    std::vector<int> totalAlocacaoTemp(N);
    int tamanho = 0;
    int rankFronteira = 0;
    while(tamanho < N)
    {
        std::vector<std::pair<double, int>> distancia;
        for(int i=0; i<2*N; i++)
        {
            if(rank[i] == rankFronteira)
            {
                distancia.push_back({0.0, i});
            }
        }
        if(tamanho + distancia.size() <= N)
        {
            for(int i=0; i<distancia.size(); i++)
            {
                int indice = distancia[i].second;
                totalAlocacaoTemp[tamanho] = totalAlocacao[indice];
                std::copy(alocacao[indice].begin(), alocacao[indice].end(), alocacaoTemp[tamanho].begin());
                std::copy(totalCobertura[indice].begin(), totalCobertura[indice].end(), totalCoberturaTemp[tamanho].begin());
                tamanho ++;
            }
        }
        else
        {
            distanciaDeMultidao(distancia);
            std::sort(distancia.rbegin(), distancia.rend());
            int quantidade = N-tamanho;
            for(int i=0; i<quantidade; i++)
            {
                int indice = distancia[i].second;
                totalAlocacaoTemp[tamanho] = totalAlocacao[indice];
                std::copy(alocacao[indice].begin(), alocacao[indice].end(), alocacaoTemp[tamanho].begin());
                std::copy(totalCobertura[indice].begin(), totalCobertura[indice].end(), totalCoberturaTemp[tamanho].begin());
                tamanho ++;
            }
            break;
        }
        rankFronteira ++;
    }
    std::copy(alocacaoTemp.begin(), alocacaoTemp.end(), alocacao.begin()+N);
    std::copy(totalAlocacaoTemp.begin(), totalAlocacaoTemp.end(), totalAlocacao.begin()+N);
    std::copy(totalCoberturaTemp.begin(), totalCoberturaTemp.end(), totalCobertura.begin()+N);
}

void imprimeSolucoes(int numeroExperimento)
{
    std::string dirName = "experimento " + std::to_string(numeroExperimento) + "/";
    DIR* dir = opendir(dirName.c_str());
    if (dir) {
        closedir(dir);
    } else {
        if (mkdir(dirName.c_str(), 0777) != 0) {
            std::cerr << "Error creating directory" << std::endl;
        }
    }
    for(int i=0; i<N; i++)
    {
        std::ofstream myFile(dirName + "solucao-"+std::to_string(i+1)+".txt");
        myFile << "Total de RSUs:" << std::endl;
        myFile << totalAlocacao[i+N] << std::endl;
        for(int j=0; j<numeroClusters; j++)
        {
            myFile << "Cobertura no cluster "+std::to_string(j)+":" << std::endl;
            myFile << totalCobertura[i+N][j] << std::endl;
        } 
        myFile << "Celulas com RSU:" << std::endl;
        for(int j=0; j<numeroCelulas; j++)
        {
            if(alocacao[i+N][j])
            {
                auto it = indiceParaCelula.find(j);
                if (it != indiceParaCelula.end())
                {
                    myFile << std::to_string(it->second.first)+","+std::to_string(it->second.second) << std::endl;
                }
            }
        }
        myFile.close();
    }
}

int main(int argc, char **argv)
{
    int formulacao = 1;
    std::vector<int> tau; // em segundos
    int numeroExperimento = 1;
    if(argc >= 3)
    {
        //formulacao = std::atoi(argv[1]);
        numeroExperimento = std::atoi(argv[1]);
        for(int i=2; i<argc; i++)
        {
            tau.push_back(std::atoi(argv[i]));
        }
    }
    else
    {
        char op;
        std::cout << "Utilizando formulacao " << formulacao << " e tau de " << 30 << " segundos no experimento " << numeroExperimento << ".";
        std::cout << " Prosseguir? (s/n)" << std::endl;
        std::cin >> op;
        if(op != 's')
        {
            return 0;
        }
        tau.push_back(30);
    }
    std::cout << "Iniciando..." << std::endl;
    leParametros(tau);
    inicializa(tau);
    geraPopulacaoInicial();
    avalia(formulacao, tau);
    std::copy(alocacao.begin(), alocacao.begin()+N, alocacao.begin()+N);
    std::copy(totalAlocacao.begin(), totalAlocacao.begin()+N, totalAlocacao.begin()+N);
    std::copy(totalCobertura.begin(), totalCobertura.begin()+N, totalCobertura.begin()+N);
    gravaBenchmarks(numeroExperimento);
    for(int i=0; i<G; i++)
    {
        std::cout << i+1 << "\n"; // debug
        auto beg = std::chrono::high_resolution_clock::now();
        cruzamento();
        std::cout << "mutacao\n";
        mutacao();
        std::cout << "avalia\n";
        avalia(formulacao, tau);
        std::cout << "selecao\n";
        selecao();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - beg);
        std::cout << "Duracao: " << duration.count() << std::endl; // debug
    }
    imprimeSolucoes(numeroExperimento);
    return 0;
}
