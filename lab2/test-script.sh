#!/bin/bash
for i in mydisk1 mydisk2 mydisk3; do 
    mkfs.vfat /dev/$i
    mkdir -p /mnt/$i 
    mount /dev/$i /mnt/$i 
done 

for source_dir in /mnt /mnt/mydisk1 /mnt/mydisk2 /mnt/mydisk3; do
    echo "=========="
    source=$source_dir/src.txt  
    echo "Creating $source"
    python -c "print('a' * (9 << 10 << 10))" > $source 
    for destination_dir in /mnt /mnt/mydisk1 /mnt/mydisk2 /mnt/mydisk3; do 
        if [ $source_dir == $destination_dir ]; then
            continue
        fi
        dest=$destination_dir/dest.txt
        echo "Copy to $dest"
        pv -pr $source > $dest
        rm $dest
    done
    rm $source
done

for i in mydisk1 mydisk2 mydisk3; do 
    umount /dev/$i
    rm -rf /mnt/$i
done 
