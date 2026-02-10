/*********************************************************************
 * \file   midi_input.h
 * \brief  MIDI入力管理（WinMM版）
 *********************************************************************/
#pragma once
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

//==============================================================================
// MIDIメッセージ種別
//==============================================================================
enum class MidiMessageType {
    Unknown,
    NoteOff,
    NoteOn,
    PolyPressure,
    ControlChange,
    ProgramChange,
    ChannelPressure,
    PitchBend
};

//==============================================================================
// MIDIメッセージ
//==============================================================================
struct MidiMessage {
    MidiMessageType type = MidiMessageType::Unknown;
    BYTE channel = 0;         // チャンネル（0-15）
    BYTE noteNumber = 0;      // ノート番号（0-127）
    BYTE velocity = 0;        // ベロシティ（0-127）
    BYTE ccNumber = 0;        // CC番号（0-127）
    BYTE ccValue = 0;         // CC値（0-127）
    BYTE pressure = 0;        // プレッシャー値
    BYTE program = 0;         // プログラム番号
    int pitchBend = 0;        // ピッチベンド（-8192 ~ 8191）
    DWORD timestamp = 0;      // タイムスタンプ
};

//==============================================================================
// MIDI入力状態
//==============================================================================
struct MidiInputState {
    // ノート状態（128ノート分）
    bool noteOn[128] = {};
    BYTE velocity[128] = {};

    // CC値（128個分）
    BYTE cc[128] = {};

    // ピッチベンド（-8192 ~ 8191）
    int pitchBend = 0;

    // チャンネルプレッシャー
    BYTE pressure = 0;

    // プログラム番号
    BYTE program = 0;

    // 最後のメッセージ
    MidiMessage lastMessage = {};

    // 接続状態
    bool connected = false;
};

//==============================================================================
// MIDIデバイス情報
//==============================================================================
struct MidiDeviceInfo {
    bool valid = false;
    char name[64] = {};
    WORD manufacturerId = 0;
    WORD productId = 0;
};

//==============================================================================
// MIDI入力クラス
//==============================================================================
class MidiInput {
public:
    // 初期化・終了・更新
    static bool Initialize();
    static void Finalize();
    static void Update();

    // 状態取得
    static const MidiInputState& GetState() { return s_state; }
    static const MidiMessage& GetLastMessage() { return s_state.lastMessage; }
    static MidiDeviceInfo GetDeviceInfo();
    static bool IsConnected() { return s_state.connected; }

    // ノート
    static bool IsNoteOn(BYTE note) { return s_state.noteOn[note]; }
    static BYTE GetVelocity(BYTE note) { return s_state.velocity[note]; }
    static bool IsAnyNoteOn();
    static int GetNoteOnCount();
    static int GetLowestNote();
    static int GetHighestNote();

    // CC
    static BYTE GetCC(BYTE ccNum) { return s_state.cc[ccNum]; }
    static float GetCCFloat(BYTE ccNum) { return s_state.cc[ccNum] / 127.0f; }

    // ピッチベンド
    static int GetPitchBend() { return s_state.pitchBend; }
    static float GetPitchBendFloat() { return s_state.pitchBend / 8192.0f; }

    // その他
    static BYTE GetProgram() { return s_state.program; }
    static BYTE GetPressure() { return s_state.pressure; }

    // ノート名
    static const char* GetNoteName(BYTE note);

private:
    static void CALLBACK MidiCallback(HMIDIIN hMidiIn, UINT wMsg,
        DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    static void ProcessMessage(DWORD param1, DWORD param2);

    static HMIDIIN s_hMidiIn;
    static MidiInputState s_state;
};
