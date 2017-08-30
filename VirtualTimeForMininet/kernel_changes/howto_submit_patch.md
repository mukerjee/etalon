##Setup Email
0. set default editor by
```
sudo update-alternatives --config editor
```
1. install esmtp and create ~/.esmtprc
```
identity "my.email@gmail.com"
hostname smtp.gmail.com:587
username "my.email@gmail.com"
password "ThisIsNotARealPassWord"
starttls required
```

2. install mutt and create ~/.muttrc
```
set sendmail="/usr/bin/esmtp"
set envelope_from=yes
set from="Your Name <my.email@gmail.com>"
set use_from=yes
set edit_headers=yes
```
test by running ```mutt```

3. setup git email
create/modify .gitconfig
```
[user]
   name = Your Name
   email = your.email@example.com
```
the email here must be the same as the email in esmtp

##Create My Branch
1. create by running
```
git checkout -b virtualtime-patch
```
2. see all the branches: ```git branch -a```
staging-next contains patch for **next** kernel release
staging-linus contains bug fix patches for **current** kernel release
3. see current branch: ```git branch```
4. see log for a branch: ```git log origin/staging-next```
5. see all log with compact form: ```git log --pretty=oneline --abbrev-commit

##Update and Rebase
1. fetch updates
```
git fetch origin
```
2. rebase to staging next, since we are patching not bug fixing
```
git rebase origin/staging-next
```

##Build and Install Kernel
0. some dependency to install
```
sudo apt-get install vim libncurses5-dev gcc make git exuberant-ctags
```
1. duplicate current config file on my PC
```
touch .config
cp /boot/config-`uname -r` .config
yes "" | make oldconfig
```
2. compile and install: see the ```build.all``` script 

##Add Pre-Commit Hooks
1. edit .git/hooks/pre-commit to contain the code that check if patch comiles to the kernel coding style
```
#!/bin/sh
exec git diff --cached | scripts/checkpatch.pl --no-signoff - || true
```
2. make it executable: ```chmod a+x .git/hooks/pre-commit```

##Commit Patch
1. commit. Useful commands are
```
git status
git diff --cached #see staged changes
git reset <file> #remove changes from staging area
git commit -s -v #-s will add sign-off-by line; -v will show diff
```
commit message should follow [PatchPhilosophy](http://kernelnewbies.org/PatchPhilosophy). A example here:
```
From 2c97a63f6fec91db91241981808d099ec60a4688 Mon Sep 17 00:00:00 2001
From: Sarah Sharp <sarah.a.sharp@linux.intel.com>
To: linux-doc@vger.kernel.org
Date: Sat, 13 Apr 2013 18:40:55 -0700
Subject: [PATCH] Docs: Add info on supported kernels to REPORTING-BUGS.

One of the most common frustrations maintainers have with bug reporters
is the email that starts with "I have a two year old kernel from an
embedded vendor with some random drivers and fixes thrown in, and it's
crashing".

Be specific about what kernel versions the upstream maintainers will fix
bugs in, and direct bug reporters to their Linux distribution or
embedded vendor if the bug is in an unsupported kernel.

Suggest that bug reporters should reproduce their bugs on the latest -rc
kernel.

Signed-off-by: Sarah Sharp <sarah.a.sharp@linux.intel.com>
---
 REPORTING-BUGS |   22 ++++++++++++++++++++++
 1 files changed, 22 insertions(+), 0 deletions(-)

diff --git a/REPORTING-BUGS b/REPORTING-BUGS
index f86e500..c1f6e43 100644
--- a/REPORTING-BUGS
+++ b/REPORTING-BUGS
@@ -1,3 +1,25 @@
+Background
+==========
+
+The upstream Linux kernel maintainers only fix bugs for specific kernel
+versions.  Those versions include the current "release candidate" (or -rc)
+kernel, any "stable" kernel versions, and any "long term" kernels.
+
+Please see https://www.kernel.org/ for a list of supported kernels.  Any
+kernel marked with [EOL] is "end of life" and will not have any fixes
+backported to it.
+
+If you've found a bug on a kernel version isn't listed on kernel.org,
+contact your Linux distribution or embedded vendor for support.
+Alternatively, you can attempt to run one of the supported stable or -rc
+kernels, and see if you can reproduce the bug on that.  It's preferable
+to reproduce the bug on the latest -rc kernel.
+
+
+How to report Linux kernel bugs
+===============================
+
+
 Identify the problematic subsystem
 ----------------------------------
 
-- 
1.7.9
```
2. edit git commits. Useful commands are
```
git commit --amend -v
```
which allow you to edit **HEAD** commit message. If you need to edit other commit, use
```
git rebase -i <short-commit>^
```
see more details at [GitTips](http://kernelnewbies.org/GitTips)
3. view commit
```
git show HEAD
```
and make they look nice in
```
git log
git log --pretty=oneline --abbrev-commit
```

##Send Patch
1. use ```get_maintainer.pl``` to find the maintainer. Either use commit or the filename as its input
```
git show HEAD | perl scripts/get_maintainer.pl #use commit message
perl scripts/get_maintainer.pl -f kernel/time/timekeeping.c #feed patched src code
```
2. send patch to **yourself** first!
	2.1 create formatted patch
		```
		git format-patch -o /tmp/ HEAD^ #as PATCH
		git format-patch --output-directory /tmp/ --subject-prefix RFC HEAD^ #as RFC may be better
		```
	2.2 send with ```mutt -H /tmp/0001-<last command's output filename>```























