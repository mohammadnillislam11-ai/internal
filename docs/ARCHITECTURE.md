# NINMODS — System Architecture & Execution Pipeline (`docs/ARCHITECTURE.md`)

> **ডকুমেন্ট পরিচিতি:** এই ডকুমেন্টে NINMODS ক্লায়েন্টের সম্পূর্ণ আর্কিটেকচার, এক্সিকিউশন লাইফসাইকেল, ইন্টারফেস বাউন্ডিং এবং রেফারেন্স প্রোজেক্টের সাথে সংযোগ বিশদভাবে বিশ্লেষণ করা হয়েছে।

---

## ১. হাই-লেভেল সিস্টেম ওভারভিউ (High-Level Architecture)

NINMODS ক্লায়েন্ট একটি স্ট্যান্ডঅ্যালোন নেটিভ অ্যান্ড্রয়েড এক্সিকিউটেবল (`arm64-v8a` বাইনারি `client.sh`) হিসেবে তৈরি। এটি কোনো প্রকার জাভা ফ্রেমওয়ার্ক নির্ভরতা ছাড়া সরাসরি লিনাক্স কার্নেল ও সারফেসফ্লিঙ্গার (SurfaceFlinger) এর সাথে ইন্টারঅ্যাক্ট করে।

```
+-----------------------------------------------------------------------------------+
|                           Android OS Layer (Linux Kernel)                         |
|     /dev/input/event* (Touch)   |   SurfaceFlinger (Native Display Surface)       |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                        NINMODS Standalone Client Core                             |
|                                                                                   |
|  [Entry Point: src/main.cpp]                                                      |
|   ├── Screen Resolution Query & Native Window Init (ANativeWindowCreator.h)       |
|   ├── Raw Touch Event Poller Thread (Touch.cpp)                                   |
|   ├── Security Interceptor (bypass.cpp / ShadowHook)                              |
|   └── Target Process Attach (Free Fire / Free Fire MAX)                           |
|                                                                                   |
|  [GUI & Render Engine]                                                            |
|   ├── OpenGL ES 3.0 Context & ImGui Implementation (draw.cpp)                     |
|   ├── Floating Indicator Pill: "nin" & Live FPS (bar.cpp)                         |
|   ├── Glassmorphic Dark UI & Configuration Matrix (menu.cpp, cfg.hpp)             |
|   └── 100% Manual Feature Gating Controls                                         |
|                                                                                   |
|  [Game Memory Engine: src/game/data.cpp]                                          |
|   ├── Kernel Memory Reader: process_vm_readv (memory.hpp)                         |
|   ├── BaseGameFacade ➔ BaseGame ➔ Match ➔ MonoDictionary Pipeline                 |
|   ├── Camera Chain & ViewMatrix Projection (math.hpp, vector3.h)                  |
|   └── Entity Bone Joints, HP, Armor, and Drop Loot Parser                         |
|                                                                                   |
|  [Visual ESP Engine: src/func/esp.hpp]                                            |
|   ├── WorldToScreen Coordinates Translation                                       |
|   ├── Dual-Layer Antialiased Tracer & Distance Fade                               |
|   ├── Anatomical Skeleton Bones, Bounding Boxes, Health & Armor Panels           |
|   └── Animated Pulsing Crosshair Circle                                           |
|                                                                                   |
|  [Memory Hook Engine: src/game/AimSilent.h]                                       |
|   └── Player.TakeDamage Interceptor (Manual RVA Trampoline Hook)                  |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                         Target Game Address Space                                 |
|            (com.dts.freefireth / com.dts.freefiremax : libil2cpp.so)              |
+-----------------------------------------------------------------------------------+
```

---

## ২. এন্ড-টু-এন্ড এক্সিকিউশন ও ডেটা ফ্লো (End-to-End Pipeline)

### ধাপ ১: ইনিশিয়ালাইজেশন ও সারফেস ক্রিয়েশন (`src/main.cpp`)
1. **টার্গেট নির্ধারণ:** ব্যবহারকারীর ইনপুট আর্গুমেন্ট অনুযায়ী ফ্রি ফায়ার (`com.dts.freefireth`) অথবা ফ্রি ফায়ার ম্যাক্স (`com.dts.freefiremax`) নির্বাচিত হয়।
2. **ডিসপ্লে স্ক্রিন ডিটেকশন:** `screen_config()` অ্যান্ড্রয়েডের কারেন্ট ডিসপ্লে সাইজ ও ওরিয়েন্টেশন চিহ্নিত করে (যেমন ২৪০০ x ১০৮০)।
3. **নেটিভ সারফেস ক্রিয়েশন:** `ANativeWindowCreator` ইন্টারফেসের মাধ্যমে SurfaceFlinger-এ সরাসরি একটি ফুল-স্ক্রিন ট্রান্সপারেন্ট নেটিভ উইন্ডো তৈরি করা হয়।
4. **টাচ লিসেনার ইনিট:** `touch::init()` লিনাক্স ইনপুট নোডগুলো (`/dev/input/event*`) স্ক্যান করে মাল্টি-টাচ ডিভাইস হুক করে।

### ধাপ ২: বাইপাস ও প্রসেস অ্যাটাচ (`src/bypass.cpp`, `src/other/memory.hpp`)
1. **শ্যাডোহুক ইঞ্জিন ইনিট:** `shadowhook_init()` এর মাধ্যমে ক্লায়েন্ট মেমরি হুকিং রেডি করে।
2. **অ্যান্টি-চিট স্টাব ইন্টারসেপশন:** `libanogs.so` এর ডিটেকশন থ্রেড ও অ্যান্টি-ডিবাগ ফাংশনগুলোকে নিউট্রাল জিরো স্টাব রিটার্নে হুক করা হয়।
3. **প্রসেস অ্যাটাচ:** `get_pid()` এর মাধ্যমে টার্গেট গেমের PID এবং `get_lib()` এর মাধ্যমে `libil2cpp.so` এর ভার্চুয়াল বেস অ্যাড্রেস শনাক্ত করা হয়।

### ধাপ ৩: গেম ডেটা মেমরি ইঞ্জিন (`src/game/data.cpp`)
1. **১০০% ম্যানুয়াল গেটিং চেক:**
   ```cpp
   if (!cfg::esp::enabled) {
       // মেমরি রিডিং সম্পূর্ণ বন্ধ, প্লেয়ার ও লুট বাফার শূন্য
       return;
   }
   ```
2. **পয়েন্টার চেইন রিজল্ভ:**
   `proc::lib` + `Offsets::GameFacade` ➔ `StaticFields` ➔ `BaseGame` ➔ `MatchIsRunning` ➔ `Match_PlayerDict` ➔ `MonoDictionary (Entities)`.
3. **ভিউ-ম্যাট্রিক্স ও ক্যামেরা রেজোলিউশন:**
   `localPlayer` ➔ `Player_Camera` ➔ `Camera_Follow` ➔ `Camera_IntPtr` ➔ `Camera_Matrix` থেকে ভিউ-ম্যাট্রিক্স রিড করে ৩ডি ওয়ার্ল্ড স্থানাঙ্ককে ২ডি স্ক্রিন পয়েন্টে কনভার্ট করার প্রস্তুতি নেওয়া হয়।
4. **এনটিটি বোন ও ইনফরমেশন পার্সিং:**
   প্রতিটি সক্রিয় এনিমি প্লেয়ারের হেড, ব্রেস্ট, হিপ, শোল্ডার, ফুট বোন নোড এবং এইচপি/আর্মর পুল থেকে বর্তমান লাইফ পয়েন্ট রিড করা হয়।

### ধাপ ৪: রেন্ডার লুপ ও মেনু ডিসপ্যাচ (`includes/draw/Android_draw/draw.cpp`, `src/ui/menu.cpp`)
1. **ইনপুট সিঙ্ক:** `touch::poll()` ফ্রেমের শুরুতে টাচ কোঅর্ডিনেট ImGui IO-তে পুশ করে।
2. **ভিজ্যুয়াল ড্রয়িং:** `func::esp::draw()` সক্রিয় থাকলে অন-স্ক্রিন এনটিটিগুলোর চারপাশে বক্স, স্কেলিটন, ট্র্রেসার ও নেম-প্যানেল রেন্ডার করে।
3. **ফ্লোটিং পিল ও মেনু:** `bar::render()` স্ক্রিনের একপাশে ছোট ড্র্যাগেবল "nin" পিল প্রদর্শন করে, যা ট্যাপ করলে মূল গ্লাস মরফিক ডার্ক মেনু ড্রপ-ডাউন হয়।

---

## ৩. রেফারেন্স প্রোজেক্ট আর্কিটেকচার ম্যাপিং (`refer/refer/app`)

রেফারেন্স ডিরেক্টরিতে রক্ষিত `refer/refer/app` প্রোজেক্টটি একটি ইউনিটি IL2CPP হুক প্রজেক্ট (`libPrabirxLive.so`), যা মূলত মেমরি ইন্টারঅ্যাকশন ও উইজেট মেকানিজম যাচাইয়ের জন্য ব্যবহৃত হয়:
- **`PrabirxLive.cpp`**: ImGui ফ্রেমওয়ার্কের সাথে টাচ ও ড্রয়িং সিঙ্ক্রোনাইজেশনের রেফারেন্স।
- **`MAFIA.h`**: ইলটুসিপিপি ক্লাস পয়েন্টার ও মেথড অফসেট স্ট্রাকচার।
- **`KittyMemory` & `xdl`**: রানটাইম মেমরি প্রোবিং ও ডায়নামিক লিঙ্কার সিম্বল রেজোলিউশন।
- **উপযোগিতা:** আমাদের স্ট্যান্ডঅ্যালোন ক্লায়েন্টের বাউন্ডিং বক্স স্ক্রিন ক্ল্যাম্পিং, লাইন থিকনেস এবং টাচ প্রিসিশন সরাসরি এই রেফারেন্স প্রোজেক্টের মেকানিজম থেকে নিশ্চিত করা হয়েছে।

