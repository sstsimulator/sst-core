## How to contribute to SST-Core

#### **Source Code Repository**

* SST-Core is the repository for the Sandia developed SST Simulation Core.  The repository is hosted on [GitHub](https://github.com).
   * An understanding of [git scm](https://git-scm.com/) is essential to developing code for SST.
   * There are 2 primary branches used for SST development
      * **devel** - Contains the latest offical codeset of SST-Core.  **This branch is unstable and may be broken at any time.**
      * **master** - Contains the latest fully tested stable version of SST-Core.

Repository to SST-Core is located [here](https://github.com/sstsimulator/sst-core).

---

#### **Issues**

* **Ensure the bug was not already reported** by searching on GitHub under [Issues](https://github.com/sstsimulator/sst-core/issues).

* If you're unable to find an open issue addressing the problem, [open a new one](https://github.com/sstsimulator/sst-core/issues/new). Be sure to include: 
   * A **title and clear description** 
   * As much relevant information as possible
   * A **code sample** or an **executable test case** demonstrating the expected behavior that is not occurring.
   * On the right side of the issue entry form, set the **Labels**, **Milestone** and **Assignee** as appropriate.

---

#### **Forked Development**

* To contribute code to the sst-core repo, you must create a fork of the repo and setup for forked development.

##### Creating a Fork
A fork is simply a personal copy of the repo on github for example the official sst-core repo is located here:

`https://github.com/sstsimulator/sst-core`

my fork of that repo is here:

`https://github.com/fryeguy52/sst-core`


The way to create your own fork is to click the fork button in the upper right while you are at the official repo. Once you have done that you can simply clone

```
git clone https://github.com/fryeguy52/sst-core
```

##### Getting updates from the official repo to your local clone

Your fork will not automatically get updates from the official repo but you will want to regularly get those updates, especially as you are starting a new branch. The easiest way to do this is to tell the `devel` branch in your local repo to track the `devel` branch on the official repo. First create a new remote, then point the devel branch to that remote.

```
git checkout devel
git remote add sst-official https://github.com/sstsimulator/sst-core
git pull --all
git branch devel --set-upstream-to sst-official/devel
```
You can verify that things are setup correctly

```
git remote -vv
origin	   https://github.com/fryeguy52/sst-core (fetch)
origin	   https://github.com/fryeguy52/sst-core (push)
sst-official					 https://github.com/sstsimulator/sst-core.git (fetch)
sst-official					 https://github.com/sstsimulator/sst-core.git (push)

git branch -vv
  devel  be1b790 [sst-official/devel: ahead 82, behind 61] Merge pull request #496 from sstsimulator/devel
* master be1b790 [origin/master] Merge pull request #496 from sstsimulator/devel
```
you should see that the `sst-official` is pointing to the official repo and that the `devel` branch in you local repo is pointing to `sst-official/devel`. Now all you need to do to get updates from the official sst-core repo devel branch is:

```
git checkout devel
git pull
```

Now you can branch from `devel` to create new features and when you push those feature branches they will push to your fork. When you pull devel, it will get updates from the official sst repo

##### Feature branches
Once a local clone has more than 1 remote (origin and sst-official in this case) you will need to tell git which remote to track for a new branch.  this requires one additional step when pushing a branch for the first time.  I personally like to push the branch right after I create it like shown below.

```
git checkout devel
git pull
git checkout -b my-new-feature-branch
git push --set-upstream origin my-new-feature-branch
<making changes ...>
git add <approprite files>
git commit
git push
```

once you are done with the feature and would like to get changes to the official repo, you will do that with a pull request from the branch on your fork to the `devel` branch on the official repo. This is done through the github UI.


##### More cleanup of local forked repo (optional)
You can also set your `master` branch to get updates from the official repo.

```
git checkout master
git branch master --set-upstream-to sst-official/master
git pull
```

If you desire to keep up to syncronized with the branches of the official repo, you can occasionally fetch with prune to clear out any remote-tracking references

```
git checkout devel
git fetch --all --prune
```

---

#### **Patches to existing code to fix an issue**

* Create a `issue-fix` branch on your forked repo derived from the **devel** branch  
* Make all required changes to correct the issue. All the changes must be commited to the `issue-fix` branch.
* Open a new GitHub pull request from the `issue-fix` branch to the **sst-core/devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `sst-core/devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the problem and solution. Include the relevant issue number if applicable.
   *  **DO NOT ATTEMPT TO MERGE THE `issue-fix` branch, it will be merged automatically if it passes tests.**
   *  **DO NOT DELETE THE `issue-fix` branch, until it is merged.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

---

#### **New Feature**

* Create a `new-feature` branch on your forked repo derived from the **sst-core/devel** branch  
* Make all required changes to correct the issue. All the changes must be commited to the `new-feature` branch.
* Open a new GitHub pull request from the `new-feature` branch to the **sst-core/devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `sst-core/devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the new feature, and any relevant information.
   *  **DO NOT ATTEMPT TO MERGE THE `new-feature` branch, it will be merged automatically.**
   *  **DO NOT DELETE THE `new-feature` branch, until it is merged.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

---

#### **SST AutoTester (CI Testing)**

* When a Pull Request is created against the **sst-core/devel** branch, the SST AutoTester application will automatically run (usually within 30 minutes) and will build and test the source branch of the Pull Request.  
   * Testing is performed across a number of different platforms
   * If the test suites pass, then the Pull Request will be setup for manual merging by SST-Core staff.
   * The testing is not all inclusive, it is possible for a bug related to a specific platform to slip in.  See Nightly Testing below.

---

#### **Nightly Testing**

* Every night, a full regression test of all the code will be tested across all supported platforms.  If the testing is successful, then a Pull Request from the **sst-core/devel** branch to the **sst-core/master** will be created and automatically merged.
   * The **sst-core/master** branch will always contain the latest fully tested stable version of SST-Core



