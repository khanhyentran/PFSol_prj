1. Clone a Git project
git clone https://github.com/khanhyentran/PFSol_prj
git fetch

2. Create your develop branch
git checkout -b my_dev

3. List current branches
git branch

4. Switch to a branch
git checkout branch_name

5. Add your file to your develop branch
git add file_name
git status

6. Commit to remote repository
git commit -m 'Add abc.c'

7. Push to master branch of the remote repository
git push origin develop

8. Merge develop to master
git checkout master
git merge develop

9. See history of git
git lg

