// main.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//

#include "commons.h"
#include "FamiTrackerFile.h"
#include "Converter.h"

int main(int argc, char** argv) {
    std::filesystem::path midiFile;
    std::filesystem::path txtFile;
    if (argc < 2) {
        std::cout << "Usage: MidiToFamiTrackerConverter <input midi file> [<output txt file>]" << std::endl;
        return 1;
    }
    else {
        midiFile = argv[1];
        if (argc >= 3) {
            txtFile = argv[2];
        }
        else {
            txtFile = midiFile;
            txtFile.replace_extension("txt");
        }
    }

    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
        std::cout << "BASS_Init error, code " << BASS_ErrorGetCode() << std::endl;
        return 2;
    }

	std::wcout << "Converting " << midiFile << std::endl;

    HSTREAM handle = BASS_MIDI_StreamCreateFile(false, midiFile.c_str(), 0, 0, BASS_UNICODE, 44100);
    if (handle == 0) {
        std::cout << "BASS_MIDI_StreamCreateFile error, code " << BASS_ErrorGetCode() << std::endl;
        return 3;
    }

    FamiTrackerFile file = std::make_unique<Converter>()->convert(handle);
    std::wstring title = midiFile.stem().wstring();
    if (title.length() > 31) {
        title = title.substr(0, 31); // apparently there is a char limit in the title
    }
    file.title = file.tracks[0]->name = title;
    file.exportTxt(txtFile);
    std::cout << "Successfully exported to " << txtFile << std::endl;

    return 0;
}

// Uruchomienie programu: Ctrl + F5 lub menu Debugowanie > Uruchom bez debugowania
// Debugowanie programu: F5 lub menu Debugowanie > Rozpocznij debugowanie

// Porady dotyczące rozpoczynania pracy:
//   1. Użyj okna Eksploratora rozwiązań, aby dodać pliki i zarządzać nimi
//   2. Użyj okna programu Team Explorer, aby nawiązać połączenie z kontrolą źródła
//   3. Użyj okna Dane wyjściowe, aby sprawdzić dane wyjściowe kompilacji i inne komunikaty
//   4. Użyj okna Lista błędów, aby zobaczyć błędy
//   5. Wybierz pozycję Projekt > Dodaj nowy element, aby utworzyć nowe pliki kodu, lub wybierz pozycję Projekt > Dodaj istniejący element, aby dodać istniejące pliku kodu do projektu
//   6. Aby w przyszłości ponownie otworzyć ten projekt, przejdź do pozycji Plik > Otwórz > Projekt i wybierz plik sln
