## How to contribute to SST-Core

#### **Source Code Repository**

* SST-Core is the repository for the Sandia developed SST Simulation Core.  The repository is hosted on [GitHub](https://github.com).
   * An understanding of [git scm](https://git-scm.com/) is essential to developing code for SST.
   * There are 2 primary branches used for SST development
      * **devel** - Contains the latest offical codeset of SST-Core.  **This branch may be broken at any time.**
      * **master** - Contains the latest fully tested stable version of SST-Core.

Repository to SST-Core is located [here](https://github.com/sstsimulator/sst-core).

#### **Issues**

* **Ensure the bug was not already reported** by searching on GitHub under [Issues](https://github.com/sstsimulator/sst-core/issues).

* If you're unable to find an open issue addressing the problem, [open a new one](https://github.com/sstsimulator/sst-core/issues/new). Be sure to include: 
   * A **title and clear description** 
   * As much relevant information as possible
   * A **code sample** or an **executable test case** demonstrating the expected behavior that is not occurring.
   * On the right side of the issue entry form, set the **Labels**, **Milestone** and **Assignee** as appropriate.


#### **Patches to existing code to fix an issue**

* Create a `issue-fix` branch on GitHub derived from the **devel** branch  
* Make all required changes to correct the issue. All the changes must be commited to the `issue-fix` branch.
* Open a new GitHub pull request from the `issue-fix` branch to the **devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the problem and solution. Include the relevant issue number if applicable.
   *  **DO NOT ATTEMPT TO MERGE THE `issue-fix` branch, it will be merged automatically.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

#### **New Feature**

* Create a `new-feature` branch on GitHub derived from the **devel** branch  
* Make all required changes to correct the issue. All the changes must be commited to the `new-feature` branch.
* Open a new GitHub pull request from the `new-feature` branch to the **devel** branch.
   * **CRITICAL: ENSURE THAT PULL REQUEST IS TARGETED TO THE `devel` BRANCH.**
   * Ensure the Pull Request description clearly describes the new feature, and any relevant information.
   *  **DO NOT ATTEMPT TO MERGE THE `new-feature` branch, it will be merged automatically.**
* The AutoTester tool will run and perform testing and merge the Pull Request as described below.

#### **Patches and New Features from Forks of SST-Core**

* The same process is followed as described above, only the pull request will come from the Forked branch to the sst-core/devel branch.
* The AutoTester will not automatically merge the Pull Request.  This allows the development team to review the code.

#### **SST AutoTester**

* When a Pull Request is created against the **devel** branch, an AutoTester application will automatically run (usually within 30 minutes) and will build and test the source branch of the Pull Request.  
   * Testing is implemented across a number of different platforms
   * If the test suites pass, then the Pull Request will be setup for manual merging by SST-Core admin staff.  
      * Pull Requests from forks will not be automatially merged.
   * The testing is not all inclusive, it is possible for a bug related to a specific platform to slip in.  See Nightly Testing below.

#### **Nightly Testing**

* Every night, a full regression test of all the code will be tested across all supported platforms.  If the testing is successful, then a Pull Request from the **devel** branch to the **master** will be created and automatically merged.
   * The **master** branch will always contain the latest fully tested stable version of SST-Core


