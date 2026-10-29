# NINMODS — Routing Pathways & Component Directory (`docs/ROUTING_AND_COMPONENTS.md`)

> **ডকুমেন্ট পরিচিতি:** এই ডকুমেন্টে NINMODS রিপোজিটরির প্রতিটি ফাইল, এর ভূমিকা, এক্সপোর্টেড ফাংশন, মেমরি হুক এবং সম্পূর্ণ এক্সিকিউশন রাউটিং তালিকাভুক্ত করা হয়েছে।

---

## ১. সম্পূর্ণ ফাইল ইনভেন্টরি (File Inventory)

### ১.১ রুট ও সিআই/সিডি লেয়ার
| ফাইলের নাম | ফাইলের অবস্থান | ভূমিকা ও দায়িত্ব |
| :--- | :--- | :--- |
| `README.md` | `/README.md` | প্রোজেক্ট ওভারভিউ এবং GitHub Actions রিলিজ ডাউনলোড নির্দেশিকা। |
| `build.yml` | `/.github/workflows/build.yml` | ক্লাউড-ফার্স্ট CI/CD পাইপলাইন। NDK r25c ব্যবহার করে `client.sh` তৈরি করে। |
| `docs/` | `/docs/` | আর্কিটেকচার, রুলস ও রাউটিং সংক্রান্ত প্রাতিষ্ঠানিক ডকুমেন্টেশন। |

### ১.২ মূল ক্লায়েন্ট সোর্স (`refer/ZqwClient/src/`)
| ফাইলের নাম | ফাইলের অবস্থান | ভূমিকা ও কার্যাবলি |
| :--- | :--- | :--- |
| `main.cpp` | `src/main.cpp` | ক্লায়েন্টের প্রধান এন্ট্রি পয়েন্ট। ডিসপ্লে রেজোলিউশন ডিটেক্ট করে, সারফেস ও টাচ ইনিট করে, গেমে অ্যাটাচ করে এবং মেইন রেন্ডার লুপ চালায়। |
| `bypass.cpp` | `src/bypass.cpp` | শ্যাডোহুক (ShadowHook) ব্যবহার করে `libanogs.so` এর ডিটেকশন ফাংশন হুক করে বাইপাস নিশ্চিত করে। |
| `bar.cpp` / `.hpp` | `src/ui/bar.cpp` | স্ক্রিনের একপাশে অবস্থানকারী ড্র্যাগেবল ফ্লোটিং পিল ("nin \| FPS") রেন্ডার করে। |
| `menu.cpp` / `.hpp` | `src/ui/menu.cpp` | Portfolio #16 ডার্ক গ্লাস UI মেনু। আইকন-অনলি সাইডবার, ব্রেডক্রাম্ব হেডার এবং মাল্টি-কলাম কার্ড লেআউট। |
| `theme/colors.h` | `src/ui/theme/colors.h` | Portfolio #16 ডার্ক ও ইলেকট্রিক ব্লু থিম প্যালেট এবং কালার ইন্টারপোলেশন। |
| `theme/layout.h` | `src/ui/theme/layout.h` | ডাইমেনশন মেট্রিক্স (উইন্ডো সাইজ, সাইডবার উইডথ, প্যাডিং, ইত্যাদি)। |
| `fonts/fonts.h` / `.cpp` | `src/ui/fonts/` | ডাইনামিক ফন্ট ম্যানেজার (Inter টেক্সট ফন্ট এবং Fontello আইকন ফন্ট লোডার)। |
| `fonts/fontello_data.cpp` | `src/ui/fonts/` | বাইনারি Fontello আইকন ফন্ট বাইট অ্যারে (40KB TTF ডেটা)। |
| `fonts/fonts_data.cpp` | `src/ui/fonts/` | বাইনারি Inter টেক্সট ফন্ট বাইট অ্যারে। |
| `cfg.hpp` | `src/ui/cfg.hpp` | কেন্দ্রীয় কনফিগারেশন ভ্যারিয়েবল। **সবগুলো ফিচার ডিফল্টভাবে `false` রাখা হয়েছে।** |
| `styles.h` | `src/ui/styles/styles.h` | ট্রান্সলুসেন্ট পার্পল অ্যাক্রিলিক থিম, প্যালেট, বর্ডার এবং কালার হেল্পার। |
| `esp.hpp` | `src/func/esp.hpp` | ভিজ্যুয়াল ড্রয়িং ইঞ্জিন। বক্স, স্কেলিটন, চিকন অ্যান্টি-অ্যালিয়াসড ট্রেসার, এইচপি বার, আর্মর বার এবং অ্যানিমেটেড ক্রসহেয়ার সার্কেল রেন্ডার করে। |
| `data.cpp` / `.hpp` | `src/game/data.cpp` | ব্যাকগ্রাউন্ড থ্রেড। `cfg::esp::enabled` দ্বারা গেটেড। মেমরি থেকে প্লেয়ার ও লুটের অবস্থান পার্স করে। |
| `Offsets.h` | `src/game/Offsets.h` | ফ্রি ফায়ার (OB55 / 1.132.1) `libil2cpp.so` এর একক ও কেন্দ্রীয় মেমরি অফসেট ক্যাটালগ। |
| `AimSilent.h` | `src/game/AimSilent.h` | `Player.TakeDamage` ফাংশন হুক। ম্যানুয়াল আরভিএ ইন্টারসেপশনের মাধ্যমে প্রজেক্টাইল ডিরেকশন পরিবর্তন করে। |
| `math.hpp` | `src/game/math.hpp` | ৩ডি ওয়ার্ল্ড স্থানাঙ্ককে ২ডি স্ক্রিন স্থানাঙ্কে রূপান্তরের `WorldToScreenPoint` ম্যাথমেটিক্স। |
| `memory.hpp` | `src/other/memory.hpp` | লিনাক্স কার্নেল সিস্টেম কল `process_vm_readv` এবং `process_vm_writev` এর র‍্যাপার (`rpm`, `wpm`)। |
| `utils.hpp` | `src/other/utils.hpp` | ট্রান্সফর্ম নোড ও ম্যাট্রিক্স থেকে এনটিটির ৩ডি পজিশন এক্সট্র্যাক্ট করার ইউটিলিটি (`GetNodePosition`)। |
| `vector3.h` | `src/other/vector3.h` | ৩ডি ভেক্টর, কোয়াটারনিয়ন, অয়লার অ্যাঙ্গেল এবং স্ফেরিক্যাল ইন্টারপোলেশন (`Slerp`)। |

### ১.৩ লো-লেভেল ও ডিসপ্লে ইঞ্জিন (`refer/ZqwClient/includes/`)
| ফাইলের নাম | ফাইলের অবস্থান | ভূমিকা ও কার্যাবলি |
| :--- | :--- | :--- |
| `draw.cpp` / `.h` | `includes/draw/Android_draw/` | EGL / OpenGL ES 3.0 কনটেক্সট ইনিশিয়ালাইজেশন এবং ফ্রেম ডিসপ্যাচ (`drawBegin`, `drawEnd`)। |
| `Touch.cpp` / `.hpp`| `includes/draw/Android_touch/` | `/dev/input/event*` লিনাক্স রুট ইনপুট ইভেন্ট হ্যান্ডলার এবং মাল্টি-টাচ ওরিয়েন্টেশন সিঙ্ক। |
| `ANativeWindowCreator.h`| `includes/internal/native_surface/` | সারফেসফ্লিঙ্গার (SurfaceFlinger) বাইন্ডার IPC এর মাধ্যমে নেটিভ ওভারলে উইন্ডো তৈরি করে। |

---

## ২. সম্পূর্ণ এক্সিকিউশন রাউটিং ও কন্ট্রোল চেইন (Routing Matrix)

### ২.১ অ্যাপ্লিকেশনের সূচনা ও গেম অ্যাটাচ রাউট (App Startup Route)
```
main(argc, argv)
  ├── print_banner() ➔ টার্মিনালে ক্লিন হেডার প্রদর্শন
  ├── start_overlay()
  │     ├── screen_config() ➔ ডিসপ্লে রেজোলিউশন চিহ্নিতকরণ (2400x1080)
  │     ├── initGUI_draw() ➔ ANativeWindowCreator দিয়ে সারফেস উইন্ডো তৈরি
  │     ├── touch::init() ➔ /dev/input/event* টাচ ফাইল ডিস্ক্রিপ্টর ওপেন
  │     ├── init_bypass() ➔ ShadowHook দিয়ে libanogs.so এর ডিটেকশন স্টাব
  │     ├── attach_or_launch() ➔ গেমের PID ও libil2cpp.so বেস অ্যাড্রেস শনাক্তকরণ
  │     ├── InitAimSilentHook() ➔ মেমরি রাইট পারমিশন ও হুক পয়েন্ট রেডি
  │     ├── data::thread_start() ➔ ব্যাকগ্রাউন্ড মেমরি রিডার থ্রেড স্পন
  │     └── while(true) রেন্ডার লুপ:
  │           ├── drawBegin()
  │           ├── ui::menu::render()
  │           │     ├── func::esp::draw()
  │           │     ├── bar::render() (ফ্লোটিং পিল "nin")
  │           │     └── Main Menu Dialog (গ্লাস উইন্ডো)
  │           └── drawEnd()
```

### ২.২ ইনপুট ও টাচ ইভেন্ট রাউটিং পাথ (Touch Input Routing)
```
User Screen Touch
  └── Linux Kernel Event Queue (/dev/input/event*)
        └── Touch.cpp: touch::poll()
              ├── Raw Event Parsing (ABS_MT_POSITION_X, ABS_MT_POSITION_Y)
              ├── Coordinate Clamping & Orientation Matrix Transform
              └── ImGui::GetIO().AddMousePosEvent() / AddMouseButtonEvent()
```

### ২.৩ মেমরি স্ক্যানিং ও এনটিটি পার্সিং পাইপলাইন (Memory Scan Pipeline)
```
data::loop() [Background Worker Thread]
  └── data::update()
        ├── [GATING CHECK] if (!cfg::esp::enabled) ➔ players.clear(); return; (১০০% সাইলেন্ট)
        ├── Check proc::pid > 0 && proc::lib != 0
        ├── Read GameFacade ➔ BaseGame ➔ MatchIsRunning
        │     └── if (!inMatch) ➔ players.clear(); loots.clear(); return;
        ├── Read Camera Matrix (IntPtr + 0x100) ➔ viewMatrix
        ├── Scan MonoDictionary (Active Players)
        │     ├── Filter: IsDead == false, HP > 0, enemy != localPlayer
        │     ├── Read Bone Transforms: Head, Breast, Hip, Shoulder, Ankle, Toe
        │     ├── Transform to Screen Space via boneScreen()
        │     ├── Debounce Visibility: Avatar_Uma + Uma_Visible
        │     └── Atomic Swap into data::players buffer
        └── std::this_thread::sleep_for(5ms + jitter)
```

### ২.৪ ভিজ্যুয়াল ইএসপি ডিসপ্যাচ ও রেন্ডারিং পাথ (Visual ESP Render Route)
```
ui::menu::render()
  └── func::esp::draw()
        ├── [GATING CHECK] if (!cfg::esp::enabled) return;
        ├── [LOBBY CHECK] if (!data::inMatch.load()) return;
        ├── Lock and snapshot data::players
        ├── Iterate Entities:
        │     ├── Verify on-screen bounds: ok(Head) && ok(Toe) && Head within [0..g_sw, 0..g_sh]
        │     ├── computeSkeletonBox() ➔ স্কেলিটন বাউন্ডিং বক্স
        │     ├── if (cfg::esp::tracer) ➔ টপ-সেন্টার থেকে এনিমি হেডে সিঙ্গেল আল্ট্রা-থিন লাইন (1.0f)
        │     ├── if (cfg::esp::box) ➔ drawBox() (ক্লিন কর্নার ব্র্যাকেট বা আউটলাইন বক্স)
        │     ├── if (cfg::esp::skeleton) ➔ drawSkeleton() (বোন কানেকশন ও জয়েন্ট ডট)
        │     └── if (cfg::esp::name/distance/health) ➔ drawNamePanel() (হেডের উপরে নাম, দূরত্ব ও এইচপি বার)
        └── if (cfg::esp::crosshair) ➔ drawCrosshair() (অ্যানিমেটেড পালসিং সার্কেল)
```

---

## ৩. কনফিগারেশন ও ফিচার ম্যাট্রিক্স (`cfg.hpp`)

সমস্ত ফিচার ব্যবহারকারীর প্রত্যক্ষ নিয়ন্ত্রণাধীন। মেনু থেকে চালু করলেই কাজ করবে, বন্ধ করলে সাথে সাথে সমস্ত ক্যালকুলেশন বন্ধ হবে:

| কনফিগ ভ্যারিয়েবল | টাইপ | ডিফল্ট মান | মেনু উইজেট অবস্থান | কাজ ও নিয়ন্ত্রণ |
| :--- | :--- | :--- | :--- | :--- |
| `cfg::esp::enabled` | `bool` | `false` | ESP Tab (Master Switch) | সমস্ত মেমরি স্ক্যানিং ও রেন্ডারিং এর মাস্টার গেট। |
| `cfg::esp::tracer` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Line) | টপ-সেন্টার থেকে এনিমি পর্যন্ত আল্ট্রা-থিন সিঙ্গেল ক্রিস্প লাইন (1.0f)। |
| `cfg::esp::box` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Box) | এনিমির চারপাশে ২ডি বাউন্ডিং বক্স রেন্ডার করে। |
| `cfg::esp::skeleton` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Skeleton) | অ্যানাটমিক্যাল বোন ফ্রেম ড্র করে। |
| `cfg::esp::health` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Health) | বর্তমান এইচপি ইন্ডিকেটর বার। |
| `cfg::esp::name` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Name) | এনিমির নাম প্লেট প্রদর্শন করে। |
| `cfg::esp::distance` | `bool` | `false` | ESP Tab ➔ Toggles (ESP Distance) | স্থানীয় প্লেয়ার থেকে দূরত্বের মিটার রিডিং। |
| `cfg::esp::crosshair` | `bool` | `false` | ESP Tab ➔ Toggles (Crosshair) | স্ক্রিনের সেন্টারে অ্যানিমেটেড পালসিং সার্কেল ও ক্রস। |
| `AimSilentEnabled` | `bool` | `false` | Menu ➔ Aim Silent | বুলেট ড্যামেজ প্রজেক্টাইল রিডাইরেকশন হুক। |
| `AimSilentNobita` | `bool` | `false` | Menu ➔ Aim Silent | নোবিতা স্পেশাল মোড টগল। |

