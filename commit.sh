sudo cp ../../bin/eagle.flash.bin ./bin
sudo cp ../../bin/eagle.irom0text.bin ./bin
git add .  
git status
read -p "Commit description: " desc  
git commit -m "$desc"  
git push origin master
