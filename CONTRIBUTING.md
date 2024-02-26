## How to contribute to SST-Core

#### **Source Code Repository**

* SST-Core is the repository for the Sandia developed SST Simulation Core.  The repository is hosted on [GitHub](https://github.com).
   * An understanding of [git scm](https://git-scm.com/) is essential to developing code for SST.
   * There are two primary branches used for SST development
      * **devel** - Contains the latest development codeset of SST-Core.
         * **While this branch is fairly stable, unexpected events can cause it to be broken at any time.**
      * **master** - Contains the latest fully tested stable version of SST-Core that is under development.
* Repository to SST-Core is located [here](https://github.com/sstsimulator/sst-core).
* Questions can be sent to wg-sst@sandia.gov

#### **Guiding Principle for Code Development**

Code accepted into the SST core codebase is deemed supported unless otherwise denoted to be experimental (e.g., by putting code in a directory named “experimental”, using the Experimental namespace, etc.).  Supported code is regularly tested for proper functionality and stability.  Please follow these guidelines:
 
* When a feature branch falls behind the sstsimulator/devel branch, it is preferred that the feature branch be rebased to the latest devel before being pushed to github and a PR is submitted (rebasing is preferred to merging).
   * Please ensure that there is no remerging old commits or merge commits between a forked branch and the sstsimulator:sst-core branches
* PRs will be subject to review by the SST team to ensure they adhere to the core design principles, to help reduce the invasiveness of the changes and/or to make stylistic adjustments to keep the code base as consistent as possible. Changes may be requested prior to PR acceptance.
   * SST Core is limited to language features found in C++17
   * Avoid use of static variables in core
   * Minimize need for #ifdefs
   * Prefer C++ to C APIs when otherwise equivalent
   * Avoid third-party dependencies in Core
   * Use of the SST output object is required where possible instead of printf/cout
   * Macros and other names used in preprocessor directives should be prepended with SST_ in order to avoid name clashes with other libraries
* Code formatting must adhere to the formatting specified in the .clang-format file found in the repository.  PRs will be tested using clang-format version 12 and must pass before other testing will proceed.  A script is provided in sst-core/scripts/clang-format-test.sh to help the developer do the checking.  Run without command line options, the script will check all .cc and .h files and report all instances where the format does not match.  When run with -i, the script will tell clang-format to modify failing files in-place.  It is good practice to run this script before committing any code.  Upon submitting a pull request, the code will be checked using clang-format and testing will not proceed if the formatting check fails.
* PRs will be tested using the autotester framework (abbreviated test suites) and cannot be merged until they pass.  This will test the PRs against all core and element tests (using sst-test-core and sst-test-elements scripts that are installed with SST) across most supported platforms.  The tests will also run in both serial and parallel (MPI and/or threads) configurations.
* PRs that include new features must add tests to the core test suites to test the features.  The tests will be automatically run as part of the autotester process.
* PRs that pass the autotester and fail the nightlies may be reverted until the errors are resolved.  There is typically a 2-3 day window to resolve the errors before reverting the code is considered.
* Any changes to the SST Core public API should be discussed with the core SST team ahead of time and are subject to approval by the core SST team.  Changes should be minimized and APIs should be concise. Any API that is part of an official release will be subject to the requirement of providing backward compatibility for one major release cycle after deprecation.
* Any publicly visible APIs must be documented in the header file using doxygen formatting.  The description should be sufficient for an end user to understand the purpose and functionality of the code.
* Code should endeavor to check and handle expected error conditions and provide a reasonable error/warning message to help the end user track down the error.
 


---

#### **Issues**

* **Ensure the bug was not already reported** by searching on GitHub under [Issues](https://github.com/sstsimulator/sst-core/issues).

* If you're unable to find an open issue addressing the problem, [open a new one](https://github.com/sstsimulator/sst-core/issues/new). Be sure to include:
   * A **title and clear description**
   * As much relevant information as possible
   * A **code sample** or an **executable test case** demonstrating the unexpected behavior.

---

#### **Forked Development**

To contribute code to the sst-core repo, you must create a fork of the repo and setup for forked development.

##### Creating a Fork
A fork is simply a personal copy of the repo on github for example the official sst-core repo is located here:

`https://github.com/sstsimulator/sst-core`

my fork of that repo is here:

`https://github.com/fryeguy52/sst-core`


The way to create your own fork is to click the fork button in the upper right while you are at the official repo. Once you have done that you can simply clone

```
git clone git@github.com:fryeguy52/sst-core.git
```

##### Getting updates from the official repo to your local clone

Your fork will not automatically get updates from the official repo but you will want to regularly get those updates, especially as you are starting a new branch. The easiest way to do this is to tell the `devel` branch in your local repo to track the `devel` branch on the official repo. First create a new remote, then point the devel branch to that remote.

```
git checkout devel
git remote add sst-official git@github.com:sstsimulator/sst-core.git
git pull --all
git branch devel --set-upstream-to sst-official/devel
```
You can verify that things are setup correctly

```
git remote -vv
origin       git@github.com:fryeguy52/sst-core.git (fetch)
origin       git@github.com:fryeguy52/sst-core.git (push)
sst-official git@github.com:sstsimulator/sst-core.git (fetch)
sst-official git@github.com:sstsimulator/sst-core.git (push)

git branch -vv
  devel  be1b790 [sst-official/devel: ahead 82, behind 61] Merge pull request #496 from sstsimulator/devel
* master be1b790 [origin/master] Merge pull request #496 from sstsimulator/devel
```
you should see that the `sst-official` is pointing to the official repo and that the `devel` branch in you local repo is pointing to `sst-official/devel`. Now all you need to do to get updates from the official sst-core repo devel branch is:

```
git checkout devel
git pull
```

Now you can branch from `devel` to create new features and when you push those feature branches they will push to your fork. When you pull devel, it will get updates from the official sst repo, avoiding the need to merge changes in devel into your local fork

##### Feature branches
Once a local clone has more than 1 remote (origin and sst-official in this case) you will need to tell git which remote to track for a new branch.  This requires one additional step when pushing a branch for the first time.  I personally like to push the branch right after I create it like shown below.

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

If you desire to keep synchronized with the branches of the official repo, you can occasionally fetch with prune to clear out any remote-tracking references

```
git checkout devel
git fetch --all --prune
```

---

#### **Patches to existing code to fix an issue**

* Create a "Bug Report" issue in the github issue tracker describing the bug.  This issue can be closed once the bug fix has been merged into devel. NOTE: Bug fixes are generally not subject to the same twe week waiting period as new features (see "New Feature" secion below), but could be if the fix requires major code changes.
* Create an `issue-fix` branch on your forked repo derived from the **sstsimulator:sst-core/devel** branch
  * Use the instructions above under "Forked Development" to ensure your branch is configured correctly
* Make all required changes to correct the issue. All the changes must be commited to the `issue-fix` branch.
* Open a new GitHub pull request from the `issue-fix` branch to the **sst-core/devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `sst-core/devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the problem and solution. Include the relevant issue number if applicable.
   * Ensure that the list of commits to be added matches the commit(s) you are submitting and that there are no merge commits. If you see commits such as "Merge branch 'sstsimulator:devel' into devel", re-create your branch using the "Forked Development" instructions above.
   *  **DO NOT ATTEMPT TO MERGE THE `issue-fix` branch, it will be merged via the normal workflow process.**
   *  **DO NOT DELETE THE `issue-fix` branch, until it is merged.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

---

#### **New Feature**
SST is an open source project and we encourage contribution of new features. For new features, please open a “Feature Request” in the GitHub issues tracker and describe both the use-case and the proposed functionality in sufficient detail that the community can understand how the feature will generally integrate into the existing code base.  Once the code is ready, a GitHub pull request (PR) should be opened as described in this document.  If the feature can be implemented using one of the existing “plug-in” APIs that already exist in the core, then it is preferred that the development use these APIs.  Features implemented through plug-ins can be made available externally and do not necessarily need to be merged into the main code repository, though that option is available if there is a compelling reason.
 
To support community involvement, major new features require a two week public discussion period. "Feature requests" GitHub issues must be opened at least two weeks prior to a pull request submission, and are encouraged to be opened early in the feature planning and development process. Once development of the feature starts, the "In Process" label can be applied to the issue.  This will allow the community to provide input on planning how the feature will be architected into the code base and allow other developers an opportunity to understand any potential conflicts with other feature development. This can also help prevent duplicated effort if multiple groups are developing similar functionality and can help catch potential issues early and avoid long iteration cycles on the resulting pull request before being able to merge the feature.  While we strive to include all new features developers feel they need, it is possible that the community may determine that a particular feature is not a good fit for SST, that it breaks certain fundamental assumptions of the code architecture, or that the intended implementation is not a good match for the code base.  In these cases, having early feedback can save development that will not be able to be merged.

To create a pull reqeust for the feature, use the following procedure.
* Create a `new-feature` branch on your forked repo derived from the **sstsimulator:sst-core/devel** branch
* Make all required changes to implement the new feature(s). All the changes must be commited to the `new-feature` branch.
* Open a new GitHub pull request from the `new-feature` branch to the **sstsimulator:sst-core/devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `sst-core/devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the new feature, and any relevant information.
   * Ensure that the list of commits to be added matches the commit(s) you are submitting and that there are no merge commits. If you see commits such as "Merge branch 'sstsimulator:devel' into devel", re-create your branch using the "Forked Development" instructions above.
   *  **DO NOT ATTEMPT TO MERGE THE `new-feature` branch, it will be merged via the normal workflow process.**
   *  **DO NOT DELETE THE `new-feature` branch, until it is merged.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

---

#### **Review and SST AutoTester (CI Testing)**

* When a Pull Request is created against the **sstsimulator:sst-core/devel** branch, the SST-Core developer team is notified. The team will review the request and comment on the request with any required changes.
* Once a Pull Request is approved, the request will be marked approved and the tag "AT: PRE-TEST INSPECTED" will be applied.
* At this point, the SST AutoTester application will automatically run (usually within 30 minutes) and will build and test the source branch of the Pull Request.
   * Testing is performed across a number of different platforms. See [SST AutoTesting Guide](http://sst-simulator.org/sst-docs/docs/guides/dev/autotest) for more information.
   * If the test suites pass, then the Pull Request will be setup for manual merging by the SST-Core team.
   * The testing is not all inclusive, it is possible for a bug related to a specific platform to slip in.  See Nightly Testing below.

---

#### **Nightly Testing**

* Every night, a full regression test of all the code will be tested across all supported platforms.  If the testing is successful, then a Pull Request from the **sst-core/devel** branch to the **sst-core/master** will be created and automatically merged.
   * The **sst-core/master** branch will always contain the latest fully tested stable version of SST-Core



