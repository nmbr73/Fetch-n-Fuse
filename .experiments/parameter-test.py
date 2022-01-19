# parameter-test.py
import sys

print("Here we are with "+str(len(sys.argv))+" arguments being ...")
for i, arg in enumerate(sys.argv):
    print("Argument "+str(i)+": '"+arg+"'")

# if __name__ == "__main__":
#     print(f"Arguments count: {len(sys.argv)}")
#     for i, arg in enumerate(sys.argv):
#         print(f"Argument {i:>6}: {arg}")
