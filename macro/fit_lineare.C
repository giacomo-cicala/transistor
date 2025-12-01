/*
 * Macro ROOT per l'analisi della caratteristica di uscita di un BJT PNP.
 * Autore: Gemini
 * Data: 2025
 *
 * Istruzioni:
 * 1. Assicurarsi di avere i file "caratteristica_100.txt" e "caratteristica_200.txt" nella stessa cartella.
 * 2. IMPORTANTE - Formato file richiesto dal costruttore ROOT (4 colonne):
 * Vce   Ic   errVce   errIc
 * (Nota: l'ordine è X, Y, erroreX, erroreY)
 * 3. Eseguire in terminale root con: root -l analisi_bjt.C
 */

#include <iostream>
#include <cmath>

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TMath.h"
#include "TMultiGraph.h"

void analisi_bjt()
{
    // -----------------------------------------------------
    // 1. Impostazioni grafiche
    // -----------------------------------------------------
    gStyle->SetOptFit(0); 
    gStyle->SetOptStat(0);   // Nascondi box statistica generica

    // -----------------------------------------------------
    // 2. Creazione TGraphErrors direttamente dai file
    // -----------------------------------------------------
    // Il formato stringa "%lg %lg %lg %lg" indica a ROOT di leggere 4 colonne di double
    // L'ordine interpretato è sempre: X, Y, ex, ey

    TGraphErrors *g50 = new TGraphErrors("data/50.txt", "%lg %lg %lg %lg");
    TGraphErrors *g100 = new TGraphErrors("data/100.txt", "%lg %lg %lg %lg");
    // TGraphErrors *g200 = new TGraphErrors("data/200.txt", "%lg %lg %lg %lg"); // COMMENTATO: 200 uA

    // Controllo di sicurezza se i file sono vuoti o non letti
    if (g50->GetN() == 0 || g100->GetN() == 0)
    {
        std::cout << "Errore: File non trovati o vuoti. Controlla i nomi e il formato." << std::endl;
        return;
    }

    
    // Stile Grafico 50uA (BLU)
    g50->SetTitle("Caratteristica Ib = 50 #muA; V_{CE} [V]; I_{C} [mA]");
    g50->SetMarkerStyle(20); // Cerchi pieni
    g50->SetMarkerColor(kBlue);
    g50->SetLineColor(kBlue);

    // Stile Grafico 100uA (ROSSO)
    g100->SetTitle("Caratteristica Ib = 100 #muA; V_{CE} [V]; I_{C} [mA]");
    g100->SetMarkerStyle(20); // Cerchi pieni
    g100->SetMarkerColor(kRed);
    g100->SetLineColor(kRed);

    // Stile Grafico 200uA
    // g200->SetTitle("Caratteristica Ib = 200 #muA; V_{CE} [V]; I_{C} [mA]");
    // g200->SetMarkerStyle(21); // Quadrati pieni
    // g200->SetMarkerColor(kRed);
    // g200->SetLineColor(kRed);

    // -----------------------------------------------------
    // 3. Preparazione per fit V = a + b * I nel range di V
    // -----------------------------------------------------
    // Non eseguiamo qui il fit Ic(V); l'utente desidera solo i punti sul grafico.
    // Eseguiremo invece un fit con ascissa = I_c e ordinata = V_ce per estrarre
    // la tensione di Early (a) e la conduttanza (1/b) selezionando i punti
    // con V_ce in [fitV_min, fitV_max].
    double fitV_min = 1.0;
    double fitV_max = 3.5;
    

    // -----------------------------------------------------
    // 4. Creazione Canvas e TMultiGraph (tutti i dati in unico grafico)
    // -----------------------------------------------------
    TCanvas *c1 = new TCanvas("c1", "Caratteristiche di Uscita BJT", 800, 600);
    c1->cd();
    gPad->SetGrid();

    // Creiamo un TMultiGraph per sovrapporre i dataset
    TMultiGraph *mg = new TMultiGraph();
    mg->Add(g50, "P");
    mg->Add(g100, "P");
    // mg->Add(g200, "P"); // COMMENTATO: 200 uA
    mg->SetTitle("Caratteristiche di Uscita BJT P-N-P;-V_{CE} (V);-I_{C} (mA)");
    mg->Draw("A");

    // Ridisegniamo i grafici con marker e assicuriamoci che siano visibili
    g50->Draw("P same");
    g100->Draw("P same");
    // g200->Draw("P same");

    // Non disegnare i fit sopra i dati (l'utente vuole solo i punti)

    // Legenda comune
    TLegend *leg = new TLegend(0.15, 0.70, 0.45, 0.88);
    leg->SetTextFont(42);
    leg->AddEntry(g100, "Ib=100 #muA", "lep");
    // leg->AddEntry(g200, "Ib=200 #muA", "lep");
    leg->AddEntry(g50, "Ib=50 #muA", "lep");
    leg->Draw();

    // Eseguiamo i fit V = a + b*I sui dati (asse scambiati) nel range di V richiesto
    std::cout << "\n--- Fit V = a + b*I (range V = " << fitV_min << " - " << fitV_max << " V) ---" << std::endl;

    TF1 *fitVI = new TF1("fitVI", "[0] + [1]*x", 0, 1); // range settato dinamicamente

    auto processDataset = [&](TGraphErrors *g, const char *label){
        int n = g->GetN();
        double xv, yv, exv, eyv;
        TGraphErrors *g_inv = new TGraphErrors(); // x' = I (mA), y' = V (V)
        int ip = 0;
        double minI = 1e12, maxI = -1e12;
        for (int i = 0; i < n; ++i){
            g->GetPoint(i, xv, yv);
            exv = g->GetErrorX(i);
            eyv = g->GetErrorY(i);
            // xv is V (original x), yv is I (original y)
            if (xv >= fitV_min && xv <= fitV_max){
                // swapped: x' = I, y' = V
                g_inv->SetPoint(ip, yv, xv);
                g_inv->SetPointError(ip, eyv, exv);
                if (yv < minI) minI = yv;
                if (yv > maxI) maxI = yv;
                ++ip;
            }
        }

        if (ip < 2){
            std::cout << "Dataset " << label << ": non ci sono punti sufficienti nel range V=["<<fitV_min<<","<<fitV_max<<"] V per eseguire il fit." << std::endl;
            return;
        }

        fitVI->SetRange(minI, maxI);
        fitVI->SetParameters(0.0, 1.0);
        g_inv->Fit(fitVI, "RQ"); // Range, Quiet

        double a = fitVI->GetParameter(0);
        double err_a = fitVI->GetParError(0);
        double b = fitVI->GetParameter(1);
        double err_b = fitVI->GetParError(1);
        // Stampa dei parametri di fit con errori
        std::cout << "Dataset " << label << ": fit V = a + b*I -> a = " << a << " +/- " << err_a << " V, b = " << b << " +/- " << err_b << " V/(mA)" << std::endl;

        // Early voltage: V_A = a
        double V_A = a;
        double err_V_A = err_a;

        // Conduttanza g = dI/dV = 1/b (I in mA, V in V -> g in mA/V)
        double cond_mA_per_V = 1.0 / b;
        double err_cond_mA_per_V = err_b / (b*b);

        // Converti in Siemens: 1 mA/V = 1e-3 A/V = 1e-3 S
        double cond_S = cond_mA_per_V * 1e-3;
        double err_cond_S = err_cond_mA_per_V * 1e-3;

        std::cout << "Dataset " << label << ": V_A = " << V_A << " +/- " << err_V_A << " V" << std::endl;
        std::cout << "Dataset " << label << ": conduttanza = " << cond_mA_per_V << " +/- " << err_cond_mA_per_V << " (mA/V) = " << cond_S << " +/- " << err_cond_S << " S" << std::endl;
    };

    processDataset(g50, "50 uA");
    processDataset(g100, "100 uA");

   

    if (gPad) {
        mg->GetXaxis()->SetLimits(0, 4.5);
        mg->SetMinimum(0);
        mg->SetMaximum(22);
        gPad->Modified();
        gPad->Update();
    }
}