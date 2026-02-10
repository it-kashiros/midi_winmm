/*********************************************************************
 * \file   main.cpp
 * \brief  MIDI入力デバッグ用
 *********************************************************************/
#include <cstdio>
#include <conio.h>
#include <windows.h>
#include "midi_input.h"

void ClearScreen() {
    COORD coord = { 0, 0 };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void PrintLine(const char* pStr) {
    printf("%-79s\n", pStr);
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT rect = { 0, 0, 79, 24 };
    SetConsoleWindowInfo(hConsole, TRUE, &rect);
    COORD bufSize = { 80, 25 };
    SetConsoleScreenBufferSize(hConsole, bufSize);

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hConsole, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &ci);

    if (!MidiInput::Initialize()) {
        printf("MIDIデバイスが見つかりません\n");
        printf("Enterで終了...\n");
        getchar();
        return 1;
    }

    char line[128];
    bool isRunning = true;

    while (isRunning) {
        if (_kbhit() && _getch() == 27) isRunning = false;

        MidiInput::Update();
        ClearScreen();

        const MidiInputState& s = MidiInput::GetState();
        MidiDeviceInfo dev = MidiInput::GetDeviceInfo();

        PrintLine("===============================================================================");
        PrintLine("                          MIDI INPUT DEBUG                                     ");
        PrintLine("===============================================================================");

        sprintf_s(line, sizeof(line), " Device: %s", dev.valid ? dev.name : "---");
        PrintLine(line);

        PrintLine("-------------------------------------------------------------------------------");

        // 最後のメッセージ
        const MidiMessage& m = s.lastMessage;
        const char* type = "---";
        char info[48] = "";

        switch (m.type) {
        case MidiMessageType::NoteOn:
            type = "NoteOn";
            sprintf_s(info, "%s(%d) Vel:%d", MidiInput::GetNoteName(m.noteNumber), m.noteNumber, m.velocity);
            break;
        case MidiMessageType::NoteOff:
            type = "NoteOff";
            sprintf_s(info, "%s(%d)", MidiInput::GetNoteName(m.noteNumber), m.noteNumber);
            break;
        case MidiMessageType::ControlChange:
            type = "CC";
            sprintf_s(info, "CC#%d = %d", m.ccNumber, m.ccValue);
            break;
        case MidiMessageType::PitchBend:
            type = "Pitch";
            sprintf_s(info, "%d", m.pitchBend);
            break;
        case MidiMessageType::ProgramChange:
            type = "Prog";
            sprintf_s(info, "%d", m.program);
            break;
        default:
            break;
        }

        sprintf_s(line, sizeof(line), " Last: %-8s Ch:%2d  %s", type, m.channel + 1, info);
        PrintLine(line);

        PrintLine("-------------------------------------------------------------------------------");

        // 88鍵ピアノロール（2行に分割：44鍵ずつ）
        PrintLine(" Piano (A0-E4):");
        char row[80] = " ";
        for (int note = 21; note <= 64; note++) {
            int n = note % 12;
            bool black = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
            char c = s.noteOn[note] ? '#' : (black ? '.' : '_');
            row[note - 20] = c;
        }
        row[45] = '\0';
        PrintLine(row);

        PrintLine(" Piano (F4-C8):");
        strcpy_s(row, " ");
        for (int note = 65; note <= 108; note++) {
            int n = note % 12;
            bool black = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
            char c = s.noteOn[note] ? '#' : (black ? '.' : '_');
            row[note - 64] = c;
        }
        row[45] = '\0';
        PrintLine(row);

        PrintLine("-------------------------------------------------------------------------------");

        // ノート情報
        int count = MidiInput::GetNoteOnCount();
        int low = MidiInput::GetLowestNote();
        int high = MidiInput::GetHighestNote();

        if (count > 0) {
            sprintf_s(line, sizeof(line), " Notes: %d  Low: %s(%d)  High: %s(%d)",
                count, MidiInput::GetNoteName(low), low, MidiInput::GetNoteName(high), high);
        } else {
            sprintf_s(line, sizeof(line), " Notes: 0");
        }
        PrintLine(line);

        // 押されているノート
        char notes[80] = " Active: ";
        int printed = 0;
        for (int i = 0; i < 128 && printed < 8; i++) {
            if (s.noteOn[i]) {
                char tmp[12];
                sprintf_s(tmp, sizeof(tmp), "%s ", MidiInput::GetNoteName(i));
                strcat_s(notes, sizeof(notes), tmp);
                printed++;
            }
        }
        if (printed == 0) strcat_s(notes, sizeof(notes), "---");
        PrintLine(notes);

        PrintLine("-------------------------------------------------------------------------------");

        // ピッチベンド・CC
        sprintf_s(line, sizeof(line), " Pitch: %6d | Mod(CC1): %3d | Vol(CC7): %3d | Sustain: %s",
            s.pitchBend, s.cc[1], s.cc[7], s.cc[64] >= 64 ? "ON " : "OFF");
        PrintLine(line);

        PrintLine("===============================================================================");
        PrintLine(" ESC: Exit");

        Sleep(16);
    }

    MidiInput::Finalize();

    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &ci);

    return 0;
}
