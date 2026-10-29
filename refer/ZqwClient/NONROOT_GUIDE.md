# NINMODS Client — Non-Root & UI Preview Guide

এই গাইডে বিস্তারিতভাবে বলা হয়েছে কেন Non-Root ডিভাইসে `Permission denied (error 126)` আসে, কীভাবে Non-Root ফোনে মেনু রান করবেন এবং যেকোনো সময় কীভাবে তা বন্ধ করবেন।

---

## ১. "Permission denied (error 126)" কেন আসছিল?

আপনার স্ক্রিনশটে যে এররটি এসেছে:
```text
bash: /storage/emulated/0/.../CleanOnExit/5_...sh: Permission denied
[Process completed (error 126) - press Enter]
```

এর প্রধান দুটি কারণ:
1. **Android Storage Security (`noexec`):** অ্যান্ড্রয়েডে `/storage/emulated/0/` (ফোন মেমোরি / SD Card) এ সরাসরি কোনো বাইনারি ফাইল এক্সিকিউট করা যায় না (সিস্টেমের নিরাপত্তার জন্য এটা ব্লক করা থাকে)।
2. **ELF বাইনারি ফাইল:** `client.sh` ফাইলটি সাধারণ কোনো Bash স্ক্রিপ্ট নয়—এটি C++ দিয়ে তৈরি **ELF 64-bit ARM64 এক্সিকিউটেবল বাইনারি**। তাই এটিকে কোনো এক্সিকিউটেবল ফোল্ডারে (যেমন Termux বা `/data/local/tmp`) নিয়ে রান করতে হয়।

---

## ২. Non-Root ডিভাইসে মেনু চালু করার পদ্ধতি

### পদ্ধতি ১: Termux দিয়ে (সবচেয়ে সহজ ও রিকমেন্ডেড)

১. প্লে স্টোর বা F-Droid থেকে **Termux** অ্যাপ ইনস্টল করুন।  
২. Termux ওপেন করে নিচের কমান্ডগুলো দিন:

```bash
# স্টোরেজ পারমিশন দিন (Allow পপআপ আসলে Allow দিন)
termux-setup-storage

# ডাউনলোড ফোল্ডার থেকে ফাইলটি Termux এর হোম ডিরেক্টরিতে কপি করুন
cp /sdcard/Download/client.sh $HOME/ninmods

# এক্সিকিউট পারমিশন দিন
chmod +x $HOME/ninmods

# UI Preview / Non-Root মোডে রান করুন
$HOME/ninmods ui
```

> **টিপ:** আপনি চাইলে `./ninmods` দিলেও ক্লায়েন্ট অটোমেটিক Non-Root সনাক্ত করে UI Preview মোডে মেনু ওপেন করে দেবে!

---

### পদ্ধতি ২: Shizuku / MT Manager Terminal / ADB দিয়ে

যদি আপনার ফোনে **Shizuku** অথবা **Wireless Debugging (ADB)** সেটআপ থাকে:

```bash
# ফাইলটি /data/local/tmp এ কপি করুন
cp /sdcard/Download/client.sh /data/local/tmp/ninmods

# ফুল পারমিশন দিন
chmod 777 /data/local/tmp/ninmods

# রান করুন
/data/local/tmp/ninmods ui
```

---

## ৩. যেকোনো সময় মেনু বন্ধ করার পদ্ধতি (How to Stop)

আপনি যেকোনো সময় নিচের যেকোনো একটি উপায়ে মেনু সম্পূর্ণ বন্ধ করতে পারবেন:

### উপায় ১: মেনুর ভেতর থেকে (সবচেয়ে সহজ)
১. মেনু ওপেন করুন।  
২. বাম পাশের ট্যাবের **Settings** (৩ নম্বর ট্যাব)-এ যান।  
৩. নিচে **Actions** কার্ডের মধ্যে **Unload** বাটনে ক্লিক করুন।  
   -> মেনু এবং এর সমস্ত ব্যাকগ্রাউন্ড প্রসেস সাথে সাথে ক্লোজ হয়ে যাবে।

### উপায় ২: টার্মিনাল বা কমান্ড দিয়ে
টার্মিনালে গিয়ে নিচের যেকোনো একটি কমান্ড দিলেই প্রসেস সাথে সাথে বন্ধ হয়ে যাবে:

```bash
pkill -f ninmods
```
অথবা
```bash
pkill -f client.sh
```
অথবা যে টার্মিনালে রান করছেন সেখানে কীবোর্ডের `Ctrl` চেপে ধরে `C` চাপুন (`Ctrl + C`)।

---

## ৪. নতুন আপডেটের বৈশিষ্ট্য (UI Preview Mode)

- গেম ওপেন না থাকলেও বা রুট পারমিশন না পেলেও এখন ক্লায়েন্ট ক্র্যাশ বা বন্ধ হবে না।
- অটোমেটিক **UI Preview Mode**-এ চলে যাবে, যাতে আপনি মেনুর বাটন, অ্যানিমেশন, কালার, স্লাইডিং এবং ড্র্যাগিং টেস্ট করতে পারেন।
