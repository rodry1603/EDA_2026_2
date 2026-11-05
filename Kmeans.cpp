#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <cmath>
using namespace std;

const int K = 3;
const int MEDIDAS = 4;
const int EXPERIMENTOS = 10;

// Una fila contiene largo y ancho del sepalo, y largo y ancho del petalo (cm).
vector<vector<double>> cargarIris(const string& nombreArchivo);

struct Resultado {
    vector<vector<double>> centros;
    vector<int> grupo;          // grupo[i] indica a que centro pertenece la flor i.
    vector<double> wssGrupo;
    double wssTotal;
};

double distanciaCuadrada(const vector<double>& a, const vector<double>& b) {
    double suma = 0;
    for (int j = 0; j < MEDIDAS; j++) {
        double diferencia = a[j] - b[j];
        suma += diferencia * diferencia;
    }
    return suma;
}

Resultado ejecutarKMeans(const vector<vector<double>>& flores) {
    Resultado r;
    int n = flores.size();
    r.grupo = vector<int>(n, -1);

    // 1. Elegir al azar tres flores con medidas distintas como centros iniciales. 
    while (r.centros.size() < K) {
        int indice = rand() % n;
        bool repetida = false;
        for (const vector<double>& centro : r.centros) {
            if (flores[indice] == centro) {
                repetida = true;
            }
        }
        if (!repetida) {
            r.centros.push_back(flores[indice]);
        }
    }

    bool huboCambios = true;
    while (huboCambios) {
        huboCambios = false;
        vector<vector<double>> sumas(K, vector<double>(MEDIDAS, 0));
        vector<int> cantidad(K, 0);

        // 2. Comparar cada flor con los tres centros y elegir el mas cercano.
        for (int i = 0; i < n; i++) {
            int cercano = 0;
            double menor = distanciaCuadrada(flores[i], r.centros[0]);
            for (int g = 1; g < K; g++) {
                double distancia = distanciaCuadrada(flores[i], r.centros[g]);
                if (distancia < menor) {
                    menor = distancia;
                    cercano = g;
                }
            }
            if (r.grupo[i] != cercano) {
                r.grupo[i] = cercano;
                huboCambios = true;
            }
            cantidad[cercano]++;
            for (int j = 0; j < MEDIDAS; j++) {
                sumas[cercano][j] += flores[i][j];
            }
        }

        // 3. Promediar cada medida. Un grupo vacio conserva su centro anterior.
        for (int g = 0; g < K; g++) {
            if (cantidad[g] > 0) {
                for (int j = 0; j < MEDIDAS; j++) {
                    r.centros[g][j] = sumas[g][j] / cantidad[g];
                }
            }
        }
        // 4. Si ninguna flor cambio de grupo, el ciclo termina.
    }

    r.wssGrupo = vector<double>(K, 0);
    r.wssTotal = 0;
    for (int i = 0; i < n; i++) {
        int g = r.grupo[i];
        r.wssGrupo[g] += distanciaCuadrada(flores[i], r.centros[g]);
    }
    for (int g = 0; g < K; g++) {
        r.wssTotal += r.wssGrupo[g];
    }
    return r;
}

void guardarExperimento(ofstream& archivo, int numero, const Resultado& r) {
    archivo << "Experimento " << numero << "\nWSS total = " << r.wssTotal << "\n";
    archivo << "Centroide (largo sepalo, ancho sepalo, largo petalo, ancho petalo) en cm\n";
    for (int g = 0; g < K; g++) {
        archivo << "Grupo " << g + 1 << "  Centroide:";
        for (int j = 0; j < MEDIDAS; j++) {
            archivo << " " << r.centros[g][j];
        }
        archivo << "  WSS = " << r.wssGrupo[g] << "\n";
    }

    archivo << "Flor\tGrupo\n";
    for (int i = 0; i < (int)r.grupo.size(); i++) {
        archivo << i + 1 << "\t" << r.grupo[i] + 1 << "\n";
    }
    archivo << "\n";
}

// Exportar las dos medidas del petalo para graficarlas con Python.
void guardarGrafico(ofstream& archivo, const vector<vector<double>>& flores,
    const Resultado& r, int numero, const string& caso) {
    for (int i = 0; i < (int)flores.size(); i++) {
        archivo << caso << "," << numero << "," << r.wssTotal << ",punto,"
            << r.grupo[i] + 1 << "," << flores[i][2] << "," << flores[i][3] << "\n";
    }
    for (int g = 0; g < K; g++) {
        archivo << caso << "," << numero << "," << r.wssTotal << ",centroide,"
            << g + 1 << "," << r.centros[g][2] << "," << r.centros[g][3] << "\n";
    }
}
int main() {
    srand(time(0));
    vector<vector<double>> flores = cargarIris("iris.csv");
    if (flores.empty()) {
        return 1;
    }
    cout << "Flores leidas: " << flores.size() << "\n";
    vector<Resultado> resultados;
    ofstream informe("resultados_kmeans.txt");
    ofstream grafico("datos_grafico.csv");
    if (!informe || !grafico) {
        cerr << "No se pudieron crear los archivos de salida.\n";
        return 1;
    }
    cout << fixed << setprecision(4);
    informe << fixed << setprecision(8);
    grafico << fixed << setprecision(8);
    grafico << "caso,experimento,wss,tipo,grupo,largo_petalo,ancho_petalo\n";

    int mejor = 0;
    int peor = 0;
    for (int e = 0; e < EXPERIMENTOS; e++) {
        resultados.push_back(ejecutarKMeans(flores));
        guardarExperimento(informe, e + 1, resultados[e]);
        cout << "Experimento " << e + 1 << "  WSS = " << resultados[e].wssTotal << "\n";
        if (resultados[e].wssTotal < resultados[mejor].wssTotal) {
            mejor = e;
        }
        // En un empate por el maximo, mostrar el ultimo de los empatados.
        if (resultados[e].wssTotal >= resultados[peor].wssTotal) {
            peor = e;
        }
    }

    guardarGrafico(grafico, flores, resultados[mejor], mejor + 1, "menor");
    guardarGrafico(grafico, flores, resultados[peor], peor + 1, "mayor");
    cout << "Menor WSS: experimento " << mejor + 1 << "\nMayor WSS: experimento " << peor + 1 << "\n";
    cout << "Resultados guardados en resultados_kmeans.txt y datos_grafico.csv.\n";
    cout << "Para ver los graficos, ejecuta: python graficar_kmeans.py\n";
    return 0;
}


vector<vector<double>> cargarIris(const string& nombreArchivo) {
    ifstream archivo(nombreArchivo);
    vector<vector<double>> flores;
   /* if (!archivo) {
        cerr << "No se pudo abrir " << nombreArchivo << ".\n";
        return flores;
    }*/

    string linea;
    int numeroLinea = 0;
    while (getline(archivo, linea)) {
        numeroLinea++;
        /*if (linea.find_first_not_of(" \t\r") == string::npos) {
            continue;
        }*/

        stringstream fila(linea);
        vector<double> medidas;
        for (int j = 0; j < MEDIDAS; j++) {
            string campo;
            getline(fila, campo, ',');
            stringstream numero(campo);
            double valor;
            char sobrante;
            if (!(numero >> valor) || (numero >> sobrante) || !isfinite(valor)) {
                cerr << "Medida invalida en la linea " << numeroLinea << ".\n";
                return vector<vector<double>>();
            }
            medidas.push_back(valor);
        }
        flores.push_back(medidas);
    }


    vector<vector<double>> distintas;
    for (int i = 0; i < (int)flores.size() && distintas.size() < K; i++) {
        bool repetida = false;
        for (int j = 0; j < (int)distintas.size(); j++) {
            if (flores[i] == distintas[j]) {
                repetida = true;
            }
        }
        if (!repetida) {
            distintas.push_back(flores[i]);
        }
    }
    if (distintas.size() < K) {
        cerr << "El CSV debe contener al menos tres flores con medidas distintas.\n";
        return vector<vector<double>>();
    }
    return flores;
}
