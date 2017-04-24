#include <iostream>
#include <thread>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <random>

#define MAX_VISITANTE 20
#define TEMPO_VOLTA   5
#define TEMPO_MINIMO  1
#define TEMPO_MAXIMO  10
#define MAX_VOLTAS    5
#define MAX_PASSAGEIROS    4

//Para M = 1 carrinho
std::mutex mu;
std::mutex filaMu;
std::mutex carrinhoEsperaEncherMu;
std::mutex esperaVoltaMu;
std::mutex contadorPassageiroMu;
std::mutex contadorFilaMu;
//
std::condition_variable cvFila;
std::condition_variable cvCarrinhoEsperaEncher;
std::condition_variable cvEsperaVolta;

//
std::atomic<bool> closedPark;
std::atomic<int> contadorPassageiro; // Passageiro no carrinho
std::atomic<int> contadorFila; // Pessoas na fila
std::atomic<int> volta;


void shared_msg(std::string msg, int i)
{
	mu.lock();
	std::cout << msg << i << std::endl;
	mu.unlock();
}
//
//
//-- Passageiro/Visitante --//
//
void esperaFila(int i)
{
  std::unique_lock<std::mutex> lk(filaMu);
  shared_msg("Thread na fila: ", i);
  contadorFila++;
  cvFila.wait(lk); // Espera carrinho aprontar
}
//
//
void entraCarro(int i)
{
  std::unique_lock<std::mutex> lk(carrinhoEsperaEncherMu);

  shared_msg("Thread entrou no carrinho: ", i);
  contadorPassageiro++; // Incremente numero passageiro
  contadorFila--;

  cvCarrinhoEsperaEncher.wait(lk);
}
//
//
void esperaVoltaAcabar(int i) {
  std::unique_lock<std::mutex> lk(esperaVoltaMu);
  shared_msg("Thread rodando: ", i);
  //sleep(TEMPO_VOLTA); // Espera 20s para volta acabar
  cvEsperaVolta.wait(lk);
}
//
//
void saiCarro(int i)
{
  shared_msg("Thread saiu do carrinho: ", i);
  contadorPassageiro--; // Decrementa numero de passageiros
}
//
//
void visitantePasseia(int i)
{
  shared_msg("Thread passeando por ai: ", i);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(TEMPO_MINIMO, TEMPO_MAXIMO);
  sleep(dis(gen)); // Gera aleatorio para passeio
}
//
//
//-- Carrinho --//
//
void esperaEncher()
{
  while(contadorPassageiro < MAX_PASSAGEIROS) {
  	cvFila.notify_one(); //chama proximo da fila ate encher
		sleep(1);
  }
	// shared_msg("carrinho cheio. Total: ", contadorPassageiro);
  cvCarrinhoEsperaEncher.notify_all(); // avisa que ta cheio
}
//
//
void daVolta ()
{
  sleep(TEMPO_VOLTA); // espera o carrinho dar a volta
  cvEsperaVolta.notify_all(); // avisa pros passageiros que volta acabou
}
//
//
void esperaEsvaziar()
{
  while(contadorPassageiro != 0) ;
}
//
//
void visitante(int i)
{
  while (!closedPark)
  {
    esperaFila(i);
		if(closedPark) return;

    entraCarro(i);
		if(closedPark) return;

    esperaVoltaAcabar(i);
		if(closedPark) return;

    saiCarro(i);
		if(closedPark) return;

    visitantePasseia(i);
		if(closedPark) return;
  }
}
//
//
void carrinFunc()
{
  while (1)
  {
    esperaEncher();
    daVolta();
    esperaEsvaziar();
    volta++;
    if (volta>=MAX_VOLTAS)
    {
      closedPark = true;
			cvFila.notify_all();
			cvCarrinhoEsperaEncher.notify_all();
			cvEsperaVolta.notify_all();
			break;
    }
  }
}
//
//-- MAIN --//
//

int main ()
{
  volta = 0;
  std::thread carrinTh = std::thread(&carrinFunc); // Carrin thread
  std::vector<std::thread> visitanteList; // Cria lista de threads visitantes
  for (int i = 0; i<MAX_VISITANTE; i++)
  {
    std::thread th = std::thread(&visitante, i);
    visitanteList.push_back(std::move(th));
    assert(!th.joinable());
  }
  std::cout << "Hi from main\n";
  std::for_each(visitanteList.begin(), visitanteList.end(),[](std::thread & th)
  {
    assert(th.joinable());
    th.join();
  });
  carrinTh.join();
  return 0;
}
