for i in $(find . -type f ! -name "*.*")
do
    riscv64-linux-gnu-objdump -S $i > ../../../dump/$i.txt || true
done

for i in $(find . -name "*.exe")
do
    riscv64-linux-gnu-objdump -S $i > ../../../dump/$i.txt || true
done