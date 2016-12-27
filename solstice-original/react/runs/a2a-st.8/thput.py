total = 0
for line in open('dmp'):
    fields = line.split()
    nfield = len(fields)
    sz = int(fields[nfield-1])
    if sz > 1000:
        total += sz
print total
