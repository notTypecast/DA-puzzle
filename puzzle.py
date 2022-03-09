from copy import deepcopy
from sys import argv

N = int(argv[1])

def toggle_bit(bit):
	return 1 if not bit else 0

all_transitions = set()

for i in range(N):
	for j in range(N):
		all_transitions.add((i, j))

# Node: represents an NxN 2D matrix of 0s or 1s
# Keeps a pointer to parent node
# Transition operator is defined as: toggle bit (i, j), as well as left, right, top and bottom bits (if applicable)
# Goal test: matrix of all 1s, using transition operators
class Node:
	def __init__(self, matrix, parent, parent_transition, final_state=None, depth=None):
		if matrix is None:
			self.matrix = [[0]*N for _ in range(N)]
		else:
			self.matrix = matrix


		self.parent = parent
		self.parent_transition = parent_transition
		self.final_state = final_state
		self.depth = depth

	# Creates a new node, containing resulting matrix after transition on coordinates (i, j)
	# Returns that node and appends it to its children
	def transition(self, coordinates):
		if coordinates[0] not in range(N) or coordinates[1] not in range(N):
			return None

		new_matrix = deepcopy(self.matrix)

		new_matrix[coordinates[0]][coordinates[1]] = toggle_bit(new_matrix[coordinates[0]][coordinates[1]])

		if coordinates[0]-1 >= 0:
			new_matrix[coordinates[0]-1][coordinates[1]] = toggle_bit(new_matrix[coordinates[0]-1][coordinates[1]])

		if coordinates[0]+1 < N:
			new_matrix[coordinates[0]+1][coordinates[1]] = toggle_bit(new_matrix[coordinates[0]+1][coordinates[1]])

		if coordinates[1]-1 >= 0:
			new_matrix[coordinates[0]][coordinates[1]-1] = toggle_bit(new_matrix[coordinates[0]][coordinates[1]-1])

		if coordinates[1]+1 < N:
			new_matrix[coordinates[0]][coordinates[1]+1] = toggle_bit(new_matrix[coordinates[0]][coordinates[1]+1])

		new_node = Node(new_matrix, self, coordinates, self.final_state, depth=(None if self.depth is None else self.depth+1))

		return new_node

	# Returns True if state is final, else False
	def is_final(self):
		if self.final_state is None:
			res = True
			for row in self.matrix:
				res &= all(row)
				if not res:
					return False

			return True
		else:
			return self.matrix == self.final_state

	def __hash__(self):
		s = ""

		for row in self.matrix:
			for char in row:
				s += str(char)

		return hash(s)

	def __repr__(self):

		s = ""

		for row in self.matrix:
			for item in row:
				s += str(item) + " "
			s += "\n"

		return s

	@staticmethod
	def path_to_root(node):
		nodes = []

		n = node

		while n is not None:
			nodes.append(n)
			n = n.parent

		nodes.reverse()

		return nodes


# BFS for above problem
# BFS will not actually be able to solve this, since the minimum required steps for a solution are 15
# BFS is O(b^d), where b is breadth (here, 25) and d is depth (15), so there are over 9.3e20 transitions to go through
def BFS(start_node):
	if start_node.is_final():
		return start_node

	visited = set((hash(start_node),))
	queue = [start_node]

	while queue:
		s = queue.pop(0)

		print(f"\rDepth: {s.depth}, visited set length: {len(visited)}", end="", flush=False)

		for t_coordinates in all_transitions:
			new_node = s.transition(t_coordinates)

			if hash(new_node) in visited:
				continue

			if new_node.is_final():
				return new_node

			queue.append(new_node)
			visited.add(hash(new_node))


	return False


# Iterative Deepening (depth-first) Search for above problem
# Similar to DFS, but stops once depth initial_depth is reached and explores an entirely new branch from start
def IDS(start_node, initial_depth, final_depth):
	# depth limited search
	def DLS(node, depth):
		if node.is_final():
			return node
		
		if not depth:
			return None

		available_transitions = iter(all_transitions)

		while True:
			try:
				next_coordinates = next(available_transitions)
				next_node = node.transition(next_coordinates)
			except StopIteration:
				break

			res = DLS(next_node, depth-1)

			if res is not None:
				return res

		return None


	for i in range(initial_depth, final_depth):
		print(f"\rDepth: {i}", end="")
		node = DLS(start_node, i)

		if node is not None:
			return node

	return None

# BestFS for above problem
# Uses h as its heuristic function
# h must be a function that can be applied to an NxN matrix, which returns an estimate for the required number of steps left to solve problem
def BestFS(start_node, h):
	if start_node.is_final():
		return start_node

	visited = set((hash(start_node),))
	queue = [(start_node, h(start_node))]

	while queue:
		s = min(queue, key=lambda queue_item: queue_item[1])
		queue.remove(s)
		#print(s[0], s[1])

		print(f"\rDepth: {s[0].depth}" + " "*10, end="", flush=False)

		for t_coordinates in all_transitions:
			new_node = s[0].transition(t_coordinates)

			if hash(new_node) in visited:
				continue

			if new_node.is_final():
				return new_node

			queue.append((new_node, h(new_node)))
			visited.add(hash(new_node))


	return False


# Heuristic function for a node matrix
# Algorithm
# 1)total=0
# 2)for each 0 in matrix:
# 3) total+=1
# 4) for each 1 above, below, to the left or right of that 0, total+=1
# 5)return total
def h2(node):
	total = 0
	for i in range(N):
		for j in range(N):
			if node.matrix[i][j] == 0:
				total += 1
				if i-1 >= 0:
					total += node.matrix[i-1][j]
				if i+1 < N:
					total += node.matrix[i+1][j]
				if j-1 >= 0:
					total += node.matrix[i][j-1]
				if j+1 < N:
					total += node.matrix[i][j+1]

	return total


if __name__ == "__main__":

	initial_node = Node(None, None, None, depth=0)

	final_node = BFS(initial_node)
	#final_node = IDS(initial_node, 0, 16)
	#final_node = BestFS(initial_node, lambda node: sum(x.count(0) for x in node.matrix))
	print()

	if final_node is None:
		print("Solution not found")
		exit(0)

	solution = Node.path_to_root(final_node)

	steps = -1

	for node in solution:
		if node.parent_transition:
			print(f"T: {node.parent_transition} ->")
		print(node)

		steps += 1

	print(f"Solution has {steps} steps.")


