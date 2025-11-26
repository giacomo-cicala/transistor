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
    gStyle->SetOptFit(1111); // Mostra parametri fit, chi2, errori nel box
    gStyle->SetOptStat(0);   // Nascondi box statistica generica

    // -----------------------------------------------------
    // 2. Creazione TGraphErrors direttamente dai file
    // -----------------------------------------------------
    // Il formato stringa "%lg %lg %lg %lg" indica a ROOT di leggere 4 colonne di double
    // L'ordine interpretato è sempre: X, Y, ex, ey

    TGraphErrors *g100 = new TGraphErrors("caratteristica_100.txt", "%lg %lg %lg %lg");
    TGraphErrors *g200 = new TGraphErrors("caratteristica_200.txt", "%lg %lg %lg %lg");

    // Controllo di sicurezza se i file sono vuoti o non letti
    if (g100->GetN() == 0 || g200->GetN() == 0)
    {
        std::cout << "Errore: File non trovati o vuoti. Controlla i nomi e il formato." << std::endl;
        return;
    }

    // Stile Grafico 100uA
    g100->SetTitle("Caratteristica Ib = 100 #muA; V_{CE} [V]; I_{C} [mA]");
    g100->SetMarkerStyle(20); // Cerchi pieni
    g100->SetMarkerColor(kBlue);
    g100->SetLineColor(kBlue);

    // Stile Grafico 200uA
    g200->SetTitle("Caratteristica Ib = 200 #muA; V_{CE} [V]; I_{C} [mA]");
    g200->SetMarkerStyle(21); // Quadrati pieni
    g200->SetMarkerColor(kRed);
    g200->SetLineColor(kRed);

    // -----------------------------------------------------
    // 3. Fitting (Regione Attiva)
    // -----------------------------------------------------
    // Fit lineare per |Vce| >= 1V.
    // BJT PNP: Vce negativa. Regione attiva approx da -4.5V a -1.0V.
    double fit_min = -4.5;
    double fit_max = -1.0;

    // Fit espliciti del tipo a + b*x
    TF1 *f1 = new TF1("fit100", "[0] + [1]*x", fit_min, fit_max);
    TF1 *f2 = new TF1("fit200", "[0] + [1]*x", fit_min, fit_max);

    f1->SetParNames("a", "b");
    f2->SetParNames("a", "b");

    f1->SetLineColor(kCyan);
    f2->SetLineColor(kOrange + 7);

    std::cout << "\n--- Risultati FIT Ib = 100 uA ---" << std::endl;
    g100->Fit(f1, "R");
    std::cout << "Parametri fit (100 uA): a = " << f1->GetParameter(0)
              << " +/- " << f1->GetParError(0)
              << ", b = " << f1->GetParameter(1)
              << " +/- " << f1->GetParError(1) << std::endl;

    std::cout << "\n--- Risultati FIT Ib = 200 uA ---" << std::endl;
    g200->Fit(f2, "R");
    std::cout << "Parametri fit (200 uA): a = " << f2->GetParameter(0)
              << " +/- " << f2->GetParError(0)
              << ", b = " << f2->GetParameter(1)
              << " +/- " << f2->GetParError(1) << std::endl;

    // -----------------------------------------------------
    // 4. Creazione Canvas e TMultiGraph (tutti i dati in unico grafico)
    // -----------------------------------------------------
    TCanvas *c1 = new TCanvas("c1", "Caratteristiche di Uscita BJT", 1000, 800);
    c1->cd();
    gPad->SetGrid();

    // Creiamo un TMultiGraph per sovrapporre i dataset
    TMultiGraph *mg = new TMultiGraph();
    mg->Add(g100, "P");
    mg->Add(g200, "P");
    mg->SetTitle("Caratteristiche di Uscita BJT;V_{CE} [V];I_{C} [mA]");
    mg->Draw("A");

    // Ridisegniamo i grafici con marker e assicuriamoci che siano visibili
    g100->Draw("P same");
    g200->Draw("P same");

    // Disegniamo i fit sopra i dati
    f1->Draw("same");
    f2->Draw("same");

    // Legenda comune
    TLegend *leg = new TLegend(0.15, 0.70, 0.45, 0.88);
    leg->AddEntry(g100, "Dati Ib=100 #muA", "lep");
    leg->AddEntry(g200, "Dati Ib=200 #muA", "lep");
    leg->AddEntry(f1, "Fit 100 #muA", "l");
    leg->AddEntry(f2, "Fit 200 #muA", "l");
    leg->Draw();

    // -----------------------------------------------------
    // 5. Calcolo di Beta
    // -----------------------------------------------------
    // Beta = Delta_Ic / Delta_Ib calcolato dai fit a V_target

    double V_target = -3.0; // Volts

    // Valutiamo la Ic dalle funzioni di fit
    double Ic_100_val = f1->Eval(V_target);
    double Ic_200_val = f2->Eval(V_target);

    // Correnti di base (mA)
    double Ib_100 = 0.1; // 100 uA
    double Ib_200 = 0.2; // 200 uA
    double delta_Ib = Ib_200 - Ib_100;

    double delta_Ic = Ic_200_val - Ic_100_val;

    // Beta è positivo
    double beta = std::abs(delta_Ic) / delta_Ib;

    std::cout << "\n=============================================" << std::endl;
    std::cout << " CALCOLO BETA (Guadagno di corrente) a Vce = " << V_target << " V" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Ic (fit) @ 100uA: " << Ic_100_val << " mA" << std::endl;
    std::cout << "Ic (fit) @ 200uA: " << Ic_200_val << " mA" << std::endl;
    std::cout << "Delta Ic:         " << std::abs(delta_Ic) << " mA" << std::endl;
    std::cout << "Delta Ib:         " << delta_Ib << " mA" << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "BETA = " << beta << std::endl;
    std::cout << "=============================================" << std::endl;

}