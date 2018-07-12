#class which finds the min canonical form for any graph

class Canonical_Form
  def initialize
    @min_cf
  end

  #make a deep copy of the graph and flip it
  def flip_graph g
    fg = g.deep_copy
    fg.adj_list.each{|n|
      n.nbrs.reverse!
      n.edges.reverse!}
    return fg
  end

  #return the min cf using clockwise bfs over the given graph and its flip
  def find_cf g
    #g = g.deep_copy
    f = flip_graph g

    for d in 0..1
      if d == 0
        graph = g
      else
        graph = f
      end

      graph.adj_list.each {|n|
      n.nbrs.each {|nbr|
        bfi = CW_BFS.new(graph,n,graph.adj_list[nbr]).search
        cf = create_graph_from_bfi(graph,bfi)
        if @min_cf.nil?
          @min_cf = cf
        else
          compare_cf(cf)
        end}}
    end
    return @min_cf
  end


  #
  def create_graph_from_bfi(g,bfi)
    cf = g.deep_copy
    cf.adj_list.each{|n|
      n.label = bfi.nodes[n.label]
      n.nbrs.map!{|nbr| bfi.nodes[nbr]}
      n.edges.map!{|e| bfi.edges[e.abs]}
    }

    cf.adj_list.sort_by! {|n| n.label}
    cf.adj_list.each{|n|
      pos = n.nbrs.index(n.nbrs.min)
      n.shift_nbrs pos}
    return cf
  end

  def  compare_cf cf
    @min_cf.adj_list.each_index {|i|
      min_cf_node = @min_cf.adj_list[i]
      cf_node = cf.adj_list[i]

      min_cf_node.nbrs.each_index{|j|
        min_nbr = min_cf_node.nbrs[j]
        nbr = cf_node.nbrs[j]

        if min_nbr > nbr
          @min_cf = cf
        end
        if min_nbr < nbr
          return
        end }

      min_cf_node.nbrs.each_index{|j|
        min_edge = min_cf_node.edges[j]
        edge = cf_node.edges[j]

        if min_edge > edge
          @min_cf = cf
        end
        if min_edge < edge
          return
        end }}   
  end
end


class CW_BFS
  def initialize(g,r,fc)
    @graph = g
    @root = r
    @first_child = fc
    @q = []
    @parent_pos = []
  end

  def search
    bfi = BFI.new
    @q.push(@root)
    @parent_pos[@root.label] = @root.return_neighbour_pos(@first_child)
    bfi.add_node(@root)
    
    while @q.length != 0
      node = @q.shift
      node.nbrs.each_index {|i|
        nbr_node = node.return_neighbour(@graph,(@parent_pos[node.label]+i)%3)
        edge = node.return_edge(nbr_node)
        if !bfi.has_node(nbr_node)
          bfi.add_node(nbr_node)
          @q.push(nbr_node)
          @parent_pos[nbr_node.label] = nbr_node.return_neighbour_pos(node)
        end
        if !bfi.has_edge(edge)
          bfi.add_edge(edge)
        end}
    end
    return bfi
  end
  
end

class BFI
  attr_accessor :edges, :nodes, :num_nodes, :num_edges
  
  def initialize
    @edges = []
    @nodes = []
    @num_nodes = 0
    @num_edges = 0
  end

  def add_node n
    @nodes[n.label] = @num_nodes
    @num_nodes += 1
  end

  def add_edge e
    @edges[e.abs] = @num_edges
    @num_edges +=1
  end

  def has_node n
    if @nodes[n.label]
      return true
    else
      return false
    end
  end

  def has_edge e
    if @edges[e.abs]
      return true
    else
      return false
    end
  end
    
  
end
    

class Graph
  attr_accessor :adj_list, :num_edges, :num_nodes, :graph_number, :nodes_added, :org_num_nodes, :com_graphs

  def initialize num
    @adj_list = []
    @num_edges = 0
    @org_num_nodes
    @num_nodes = 0
    @graph_number = num
    @com_graphs = 0
  end

  def to_s
    s = "#{@num_nodes}"
    adj_list.each {|n| s += " #{n.num_nbrs} #{n.nbrs[0]} #{n.nbrs[1]} #{n.nbrs[2]}" }
    return s
  end
  
  def read_graph(file,i)
    contents = file[i].split(' ')
    contents.map! { |x| x.to_i }
    @num_nodes = contents[0]
    @org_num_nodes = @num_nodes
    @num_nodes.times {|x| adj_list[x] = Node.new(x,3,"n")}
    for i in 0..(@num_nodes-1)
      for j in 0..2
        adj_list[i].add_neighbour(contents[2+4*i+j])
      end
    end
  end

  def print_graph
    adj_list.each {|x| puts x}
    puts
  end


  def set_edges
    #set all edges to nil
    @adj_list.each { |n| n.edges.map! { |nbr| nil } }
    @num_edges = 0
    
    for i in 0..(@num_nodes-1) #iterate over all the nodes
      for j in 0..(@adj_list[i].num_nbrs - 1) #iterate over all the nbrs of the node
        
        if @adj_list[i].edges[j].nil? 
          node = @adj_list[i]
          nbr_node = node.return_neighbour(self,j)

          #is the node connected to same node mult x?        
          if node.contains_multiple_edges(nbr_node)
            num_me = node.number_of_multiple_edges(nbr_node)

          #walk to the first instance of an edge to nbr_node in node  
          pos = node.return_first_pos(nbr_node)

           #label the edges
          num_me.times{|m| node.edges[(pos+m) % node.num_nbrs] = @num_edges+m}
 
          #repeat for nbr, but opposite order of edge numbers
          pos = nbr_node.return_first_pos(node)

          num_me.times{|m| nbr_node.edges[(pos+m) % nbr_node.num_nbrs] = -((@num_edges-1)+num_me-m)}
          
          @num_edges += num_me
           
          else
            #there are no multiedges. label the edge the edge count and repeat for its neighbour
            index = node.return_neighbour_pos(nbr_node)
            node.edges[index] = @num_edges

            index = nbr_node.return_neighbour_pos(node)
            nbr_node.edges[index] = -@num_edges

            @num_edges += 1
    
          end 
        end       
      end
    end
  end


  def add_node(new_node)
    @adj_list[@num_nodes] = new_node
    @num_nodes += 1
  end

  def bisect
    #bisect each edge with a new c node
    for i in 0..@num_nodes-1
      node = @adj_list[i]
      for j in 0..node.num_nbrs-1
        nbr_node = node.return_neighbour(self,j)
        if nbr_node.type == "n"
          edge_num = node.edges[j]
          new_node = Node.new(@num_nodes,2,"c")
          new_node.add_neighbour(node.label)
          new_node.add_neighbour(nbr_node.label)
          add_node(new_node)
          node.replace_neighbour(nbr_node,new_node,j)
          pos = nbr_node.return_abs_edge_position(edge_num)
          nbr_node.replace_neighbour(node,new_node,pos)
        end
      end
    end
    
    #set new edge numbers
    set_edges
  end

  def add_chords(level)
    require 'debug'
    if level == @num_nodes
      if adj_list[8].nbrs.include? 11
        print_graph
      end
      @com_graphs += 1
      return
    end

    node = @adj_list[level]

    if node.num_nbrs < 3
      candidates = get_candidates node
      candidates.each {|x|
        connect_nodes(x)
        add_chords(level+1)
        disconnect_nodes(x) }
    else
      add_chords(level+1)
    end
  end

  def connect_nodes pos_arr

    node = @adj_list[pos_arr[0]]
    nbr_node = @adj_list[pos_arr[2]]

    node.num_nbrs += 1
    nbr_node.num_nbrs += 1

    
    #order the nbrs of node to insert new nbr in pos
    j = node.num_nbrs-1
    while j > pos_arr[1]
      node.nbrs[j] = node.nbrs[j-1]
      node.edges[j] = node.edges[j-1]
      j -= 1
    end
    node.nbrs[pos_arr[1]] = nbr_node.label
    node.edges[pos_arr[1]] = @num_edges

    j = nbr_node.num_nbrs-1
    while j > pos_arr[3]+1
      nbr_node.nbrs[j] = nbr_node.nbrs[j-1]
      nbr_node.edges[j] = nbr_node.edges[j-1]
      j -= 1
    end
    
    nbr_node.nbrs[pos_arr[3]+1] = node.label
    nbr_node.edges[pos_arr[3]+1] = -@num_edges

    @num_edges += 1    
  end

  def disconnect_nodes pos_arr
    node = @adj_list[pos_arr[0]]
    nbr_node = @adj_list[pos_arr[2]]
    node.remove_node(nbr_node)
    nbr_node.remove_node(node)
  end
  

  #walk around the face getting candidates
  def get_candidates node

    candidates = []

    for i in 0..node.num_nbrs-1
      first_edge = node.edges[i]
      curr_edge = first_edge
      curr_node = node.return_neighbour(self,i)
      pos_to_add_on_node = i
      first = true
      
      while curr_edge != first_edge or first
        first = false
        #if vertex is only degree 2, make it a candidate
        if curr_node != node and curr_node.num_nbrs == 2
          #find the pos of where the edge should be added on curr_node
          pos_to_add_on_nbr = curr_node.edges.index(-curr_edge)
          candidates.push([node.label,pos_to_add_on_node,curr_node.label,pos_to_add_on_nbr])
        end

        #move to the next node
        pos = curr_node.next_pos_after_edge(-curr_edge)
        curr_edge = curr_node.edges[pos]
        curr_node = curr_node.return_neighbour(self,pos)

      end
    end
    return candidates
  end

  def deep_copy
    g = Graph.new(@graph_number)
    g.num_edges = @num_edges
    g.org_num_nodes = @org_num_nodes
    g.num_nodes = @num_nodes
    g.com_graphs = @com_graphs
    @adj_list.each{|x| g.adj_list[x.label] = x.deep_copy}
    return g
  end
  
 end

class Node
  attr_accessor :label, :nbrs, :edges, :num_nbrs, :type

  def initialize(l,n=0,a="")
    @label = l
    @nbrs = []
    @edges = []
    @num_nbrs = n
    @type = a
  end

  def add_neighbour n
    nbrs.push n
  end
  
  def return_first_pos(nbr_node) #return the first instance of a neighbour in a nodes rot. system
    i = 0 
    while (@nbrs[i] == nbr_node.label) and (i < @num_nbrs)
      i += 1
    end
    while (@nbrs[i % @num_nbrs] != nbr_node.label)
      i += 1  
    end
    return (i % @num_nbrs)
  end

  def return_neighbour_pos(nbr_node)
    i = @nbrs.index {|x| x == nbr_node.label}
    if i.nil?
      return nil
    else
      return i
    end
  end

  def return_edge(nbr_node)
    i = return_neighbour_pos(nbr_node)
    return @edges[i]
  end

  def replace_neighbour(old_node,new_node,pos)
    @nbrs[pos] = new_node.label
  end

  def return_abs_edge_position(e)
    return @edges.index {|x| x.abs == e.abs}
  end


  
  def return_neighbour(g,i)
    nbr_label = @nbrs[i]
    return g.adj_list[nbr_label]
  end
  
    
  def to_s
    return "Node: #{label} #{num_nbrs} Neighbours: #{nbrs[0]} (#{edges[0]}), #{nbrs[1]} (#{edges[1]}), #{nbrs[2]} (#{edges[2]})"
  end

  def contains_multiple_edges(nbr_node)
    if number_of_multiple_edges(nbr_node) == 0
      return false
    else
      return true
    end
  end

  def number_of_multiple_edges(nbr_node)
    @nbrs.count(nbr_node.label)
  end

  def next_pos_after_edge edge
    i = (@edges.index {|e| e==edge} + 1) % @num_nbrs
    return i
  end

  def shift_nbrs i
    i.times { nbr = @nbrs.shift
      @nbrs.push nbr}
  end
    

  def remove_node node
    pos = return_neighbour_pos(node)
    @nbrs.delete_at(pos)
    @edges.delete_at(pos)
    @num_nbrs -= 1
  end

  def deep_copy
    n = Node.new(@label,@num_nbrs,@type)
    n.nbrs = [nbrs[0],nbrs[1],nbrs[2]]
    n.edges = [edges[0],edges[1],edges[2]]
    return n
  end

end


class MyFile
  attr_accessor :graphs
  def initialize file_name
    @graphs = IO.readlines(file_name)
  end
end

class Main

  def initialize fn
    @filename = fn
    @total = 0
    @total_nd = 0
  end
  
  def generate_graphs
    f = MyFile.new(@filename)
    f.graphs.each_index{|x|
      g = Graph.new x      
      g.read_graph(f.graphs,x)      
      g.set_edges
      g.bisect
      g.add_chords(g.org_num_nodes)
      @total += g.com_graphs}
  end
end

Main.new("graphs").generate_graphs

