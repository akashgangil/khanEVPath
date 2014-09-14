import matplotlib
matplotlib.use('agg')
import matplotlib.pyplot as plt

class Graph:
  basepath = '/net/hu21/agangil3/experiments/'
  def __init__(self, expId, intensities):
    frame = intensities.split('i')
    self.expId = expId
    self.x = filter(lambda x: x != 'null' and x != '', frame[0].split(' '))
    self.y = filter(lambda x: x != 'null' and x != '', frame[1].split(' '))

  def Plot(self):
    image_path = Graph.basepath + 'experiment_' + str(self.expId) +'_graph' 
    im1 = list(range(len(self.x)+1))[1:]
    plt.plot(im1, self.x, 'b-', im1, self.y, 'r-')
    plt.savefig(image_path)
    plt.clf()
    plt.close()
    print "Saved the graph"

  def Stats(self):
    f = open(Graph.basepath + 'experiment_' + str(self.expId) + '_stats.txt', 'w+')
    f.write(  'Max Frame1 Intensity: ' + str(max(self.x))        + '\n'
            + 'Max Frame2 Intensity: ' + str(max(self.y)) + '\n')
    f.close()        
