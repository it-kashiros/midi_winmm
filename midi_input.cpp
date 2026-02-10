/*********************************************************************
 * \file   midi_input.cpp
 * \brief  MIDI入力管理（WinMM版）
 *********************************************************************/
#include "midi_input.h"
#include <cstdio>
#include <cstring>

// 静的メンバ
HMIDIIN MidiInput::s_hMidiIn = nullptr;
MidiInputState MidiInput::s_state = {};

// ノート名
static const char* NOTE_NAMES[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// 初期化
bool MidiInput::Initialize() {
    s_hMidiIn = nullptr;
    s_state = {};

    // デバイスがなければ失敗
    if (midiInGetNumDevs() == 0) {
        return false;
    }

    // 最初のデバイスを開く
    MMRESULT result = midiInOpen(
        &s_hMidiIn,
        0,
        reinterpret_cast<DWORD_PTR>(MidiCallback),
        0,
        CALLBACK_FUNCTION
    );

    if (result != MMSYSERR_NOERROR) {
        s_hMidiIn = nullptr;
        return false;
    }

    s_state.connected = true;
    midiInStart(s_hMidiIn);

    return true;
}

// 終了
void MidiInput::Finalize() {
    if (s_hMidiIn != nullptr) {
        midiInStop(s_hMidiIn);
        midiInClose(s_hMidiIn);
        s_hMidiIn = nullptr;
    }
    s_state = {};
}

// 更新
void MidiInput::Update() {
    // コールバックで処理済み
}

// デバイス情報
MidiDeviceInfo MidiInput::GetDeviceInfo() {
    MidiDeviceInfo info = {};

    if (midiInGetNumDevs() == 0) {
        return info;
    }

    MIDIINCAPS caps;
    if (midiInGetDevCaps(0, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        info.valid = true;
        strcpy_s(info.name, sizeof(info.name), caps.szPname);
        info.manufacturerId = caps.wMid;
        info.productId = caps.wPid;
    }

    return info;
}

// コールバック
void CALLBACK MidiInput::MidiCallback(HMIDIIN hMidiIn, UINT wMsg,
    DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (wMsg == MIM_DATA) {
        ProcessMessage(static_cast<DWORD>(dwParam1), static_cast<DWORD>(dwParam2));
    } else if (wMsg == MIM_CLOSE) {
        s_state.connected = false;
    }
}

// メッセージ処理
void MidiInput::ProcessMessage(DWORD param1, DWORD param2) {
    BYTE status = param1 & 0xFF;
    BYTE byte1 = (param1 >> 8) & 0xFF;
    BYTE byte2 = (param1 >> 16) & 0xFF;
    BYTE channel = status & 0x0F;
    BYTE statusType = status & 0xF0;

    MidiMessage msg = {};
    msg.channel = channel;
    msg.timestamp = param2;

    switch (statusType) {
    case 0x80:  // ノートオフ
        msg.type = MidiMessageType::NoteOff;
        msg.noteNumber = byte1;
        msg.velocity = byte2;
        if (byte1 < 128) {
            s_state.noteOn[byte1] = false;
            s_state.velocity[byte1] = 0;
        }
        break;

    case 0x90:  // ノートオン
        msg.noteNumber = byte1;
        msg.velocity = byte2;
        if (byte2 > 0) {
            msg.type = MidiMessageType::NoteOn;
            if (byte1 < 128) {
                s_state.noteOn[byte1] = true;
                s_state.velocity[byte1] = byte2;
            }
        } else {
            msg.type = MidiMessageType::NoteOff;
            if (byte1 < 128) {
                s_state.noteOn[byte1] = false;
                s_state.velocity[byte1] = 0;
            }
        }
        break;

    case 0xA0:  // ポリプレッシャー
        msg.type = MidiMessageType::PolyPressure;
        msg.noteNumber = byte1;
        msg.pressure = byte2;
        break;

    case 0xB0:  // CC
        msg.type = MidiMessageType::ControlChange;
        msg.ccNumber = byte1;
        msg.ccValue = byte2;
        if (byte1 < 128) {
            s_state.cc[byte1] = byte2;
        }
        break;

    case 0xC0:  // プログラムチェンジ
        msg.type = MidiMessageType::ProgramChange;
        msg.program = byte1;
        s_state.program = byte1;
        break;

    case 0xD0:  // チャンネルプレッシャー
        msg.type = MidiMessageType::ChannelPressure;
        msg.pressure = byte1;
        s_state.pressure = byte1;
        break;

    case 0xE0:  // ピッチベンド
        msg.type = MidiMessageType::PitchBend;
        msg.pitchBend = ((byte2 << 7) | byte1) - 8192;
        s_state.pitchBend = msg.pitchBend;
        break;

    default:
        msg.type = MidiMessageType::Unknown;
        break;
    }

    s_state.lastMessage = msg;
}

// いずれかのノートがオン
bool MidiInput::IsAnyNoteOn() {
    for (int i = 0; i < 128; i++) {
        if (s_state.noteOn[i]) return true;
    }
    return false;
}

// オンのノート数
int MidiInput::GetNoteOnCount() {
    int count = 0;
    for (int i = 0; i < 128; i++) {
        if (s_state.noteOn[i]) count++;
    }
    return count;
}

// 最低音
int MidiInput::GetLowestNote() {
    for (int i = 0; i < 128; i++) {
        if (s_state.noteOn[i]) return i;
    }
    return -1;
}

// 最高音
int MidiInput::GetHighestNote() {
    for (int i = 127; i >= 0; i--) {
        if (s_state.noteOn[i]) return i;
    }
    return -1;
}

// ノート名
const char* MidiInput::GetNoteName(BYTE note) {
    static char buf[8];
    int octave = (note / 12) - 1;
    int n = note % 12;
    sprintf_s(buf, sizeof(buf), "%s%d", NOTE_NAMES[n], octave);
    return buf;
}
