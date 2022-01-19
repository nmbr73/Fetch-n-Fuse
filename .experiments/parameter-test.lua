-- parameter-test.lua

print("Here we are with ".. #arg .." arguments being ...")

for i = 1 , #arg-1 do
  print("Argument "..i..": '"..arg[i].."'")
end
